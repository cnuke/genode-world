/*
 * \brief  VFS block device backend for lwext4
 * \author Josef Soentgen
 * \date   2022-01-03
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <block/request.h>

/* local includes */
#include "block.h"


static constexpr Genode::size_t BLOCK_SIZE = 512;


static int blockdev_open(struct ext4_blockdev *)  { return EOK; }
static int blockdev_close(struct ext4_blockdev *) { return EOK; }


static int blockdev_io(struct ext4_blockdev *bdev,
                       void                 *buf,
                       uint64_t              lba,
                       uint32_t              count,
                       bool                  write)
{
	Lwext4::Block_device &bd = *reinterpret_cast<Lwext4::Block_device*>(bdev);

	Genode::Constructible<Util::Vfs_job> &job    =  bd._vfs_job;
	Genode::Entrypoint                   &ep     =  bd._vfs_env->env().ep();
	Vfs::Vfs_handle                      &handle = *bd._vfs_handle;

	if (job.constructed())
		return EIO;

	Block::Request request {
		.operation = {
			.type         = write ? Block::Operation::Type::WRITE
			                      : Block::Operation::Type::READ,
			.block_number = lba,
			.count        = count,
		},
		.success = false,
		.offset  = 0,
		.tag     = { 0 },
	};

	Vfs::file_offset const offset = BLOCK_SIZE * lba;
	Vfs::file_size   const length = BLOCK_SIZE * count;

	try {
		job.construct(handle, request, offset,
		              reinterpret_cast<char*>(buf), length);
	} catch (...) {
		Genode::error("could not construct VFS job");
		return EIO;
	}

	while (!job->completed())
		if (!job->execute())
			ep.wait_and_dispatch_one_io_signal();

	bool const succeeded = job->succeeded();
	job.destruct();

	if (!succeeded)
		Genode::error("could not ", write ? "write" : "read",
		              " lba: ", lba, " count: ", count);

	return succeeded ? EOK : EIO;
}


static int blockdev_bread(struct ext4_blockdev *bdev,
                          void                 *dest,
                          uint64_t              lba,
                          uint32_t              count)
{
	return blockdev_io(bdev, dest, lba, count, false);
}


static int blockdev_bwrite(struct ext4_blockdev *bdev,
                           void const           *src,
                           uint64_t              lba,
                           uint32_t              count)
{
	return blockdev_io(bdev, const_cast<void*>(src), lba, count, true);
}


static Vfs::Vfs_handle *open_block_device(Vfs::Env   &vfs_env,
                                          char const *device_path)
{
	using DS = Vfs::Directory_service;
	using OR = DS::Open_result;

	Vfs::Vfs_handle *handle = nullptr;

	OR open_result = vfs_env.root_dir().open(device_path, DS::OPEN_MODE_RDWR,
	                                         &handle, vfs_env.alloc());
	if (open_result != OR::OPEN_OK)
		return nullptr;

	return handle;
}


uint64_t query_block_device_size(Vfs::Env        &vfs_env,
                                  Vfs::Vfs_handle &handle,
                                  char const      *device_path)
{
	struct Sync
	{
		enum { INITIAL, QUEUED, COMPLETE } state { INITIAL };

		Vfs::Vfs_handle &h;

		Sync(Vfs::Vfs_handle &h) : h(h) { }

		bool complete()
		{
			switch (state) {
				case Sync::INITIAL:
					if (!h.fs().queue_sync(&h))
						return false;
					state = Sync::QUEUED; [[ fallthrough ]];
				case Sync::QUEUED:
					if (h.fs().complete_sync(&h) == Vfs::File_io_service::SYNC_QUEUED)
						return false;
					state = Sync::COMPLETE; [[ fallthrough ]];
				case Sync::COMPLETE:
					break;
			}
			return true;
		}
	};

	/* force proper stat information for (some) synthetic file systems */
	Sync sync { handle };
	while (!sync.complete())
		vfs_env.env().ep().wait_and_dispatch_one_io_signal();

	using DS = Vfs::Directory_service;
	using SR = DS::Stat_result;

	DS::Stat stat { };
	SR stat_result = vfs_env.root_dir().stat(device_path, stat);
	if (stat_result != SR::STAT_OK) {
		handle.close();
		return 0;
	}

	return (uint64_t)stat.size;
}


Lwext4::Block_device &Lwext4::block_init(Vfs::Env   &vfs_env,
                                         char const *device_path,
                                         uint32_t    block_size)
{
	struct Block_init_failed : Genode::Exception { };

	static Lwext4::Block_device *block_device = nullptr;

	if (block_device) {
		Genode::error("calling ", __func__, " multiple times not supported");
		throw Block_init_failed();
	}

	/* try to open block device first */
	Vfs::Vfs_handle *handle = open_block_device(vfs_env, device_path);
	if (!handle) {
		Genode::error("could not open block device '", device_path, "'");
		throw Block_init_failed();
	}

	uint64_t const size = query_block_device_size(vfs_env, *handle,
	                                              device_path);
	if (!size) {
		handle->close();
		Genode::error("block device size invalid");
		throw Block_init_failed();
	}

	block_device = static_cast<Lwext4::Block_device*>(
		vfs_env.alloc().alloc(sizeof (Lwext4::Block_device)));
	if (!block_device) {
		handle->close();
		Genode::error("could not allocate Block_device object");
		throw Block_init_failed();
	}

	uint64_t const part_size   = size;
	uint64_t const block_count = size / block_size;

	block_device->block_size  = block_size;
	block_device->_vfs_env    = &vfs_env;
	block_device->_vfs_handle = handle;

	block_device->ext4_blockdev.bdif        = &block_device->ext4_blockdev_iface;
	block_device->ext4_blockdev.part_offset = 0;
	block_device->ext4_blockdev.part_size   = part_size;

	block_device->ext4_blockdev_iface.ph_bbuf  = block_device->ext4_block_buffer;
	block_device->ext4_blockdev_iface.ph_bcnt  = block_count;
	block_device->ext4_blockdev_iface.ph_bsize = block_size;

	block_device->ext4_blockdev_iface.bread  = blockdev_bread;
	block_device->ext4_blockdev_iface.bwrite = blockdev_bwrite;
	block_device->ext4_blockdev_iface.close  = blockdev_close;
	block_device->ext4_blockdev_iface.open   = blockdev_open;

	return *block_device;
}


void Lwext4::block_deinit(Vfs::Env &vfs_env, Block_device &block_device)
{
	if (block_device._vfs_handle)
		block_device._vfs_handle->close();

	vfs_env.alloc().free(&block_device, sizeof (Lwext4::Block_device));
}

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

/* local includes */
#include "block.h"

static uint64_t set_lru(void)
{
	static uint64_t count = 0;
	return ++count;
}


template <uint32_t N>
struct Util::Cache
{
	typedef uint64_t (*lru_func)(void);

	struct Stats
	{
		uint32_t entries;
		size_t   chunk_size;
		size_t   block_size;

		uint64_t hits;
		uint64_t misses;
		uint64_t fills;
		uint64_t flushes;
		uint64_t reads;
		uint64_t writes;
		uint32_t used;
		uint32_t dirty;
		uint32_t last_used;
		uint32_t last_dirty;
	};

	Stats _stats { 0, 0, 0, 0, 0, 0,
	               0, 0, 0, 0, 0, 0 };

	struct Hit
	{
		uint32_t index;
		bool     valid;
	};

	struct Entry
	{
		uint64_t lba;
		uint32_t count;
	};

	using addr_t = Util::addr_t;

	Genode::Allocator &_alloc;

	lru_func _lru_fn;

	uint32_t const _num;
	addr_t _base;
	size_t _size;
	size_t const _chunk_size;
	size_t const _block_size;
	size_t const _entr { _chunk_size / _block_size };
	size_t const _mask { _entr - 1 };

	Util::Bitmap<N> _index_map { };
	Entry    _entries[N];
	uint64_t _entries_lru[N];

	enum { DM_ENTRIES = 128 };
	Util::Bitmap<DM_ENTRIES> _dirtymap[N] { };

	uint32_t _indices[N] { };
	uint32_t _dirty_indices[N] { };

	int64_t _get_offset(uint32_t index, uint64_t lba)
	{
		int64_t const i = (lba & _mask);
		int64_t const offset = index * _chunk_size + i * _block_size;
		if (offset < 0 || (size_t)offset > _size) {
			struct Invalid_offset { };
			throw Invalid_offset();
		}
		return offset;
	}

	uint64_t _last_lru { 0 };

	int _flush_entry(Util::Io &io, uint32_t index)
	{
		int64_t      const offset = _chunk_size * index;
		size_t       const length = _entries[index].count * _block_size;
		uint64_t     const lba    = _entries[index].lba;

		using namespace Util;

		Io::Buffer const iob {
			.base   = reinterpret_cast<uint8_t*>(_base + offset),
			.length = length };
		Io::At     const at  { .value = (off_t)(lba * _block_size) };

		Io::Write_result const res = io.write(iob, at);
		if (res != Io::Write_result::WRITE_OK)
			return -1;

		_stats.flushes++;

		_dirtymap[index].reset();
		return 0;
	}

	void _update(uint32_t index, uint64_t lba, bool mark_dirty)
	{
		if (mark_dirty)
			_dirtymap[index].set(lba & _mask);

		_last_lru = _lru_fn();
		_entries_lru[index] = _last_lru;
	}

	Cache(Genode::Allocator &alloc, lru_func fn,
	      uint32_t num, size_t chunk_size, size_t block_size)
	:
		_alloc  { alloc },
		_lru_fn { fn },
		_num    { num > N ? N : num },
		_base   { 0 },
		_size   { 0 },
		_chunk_size { chunk_size },
		_block_size { block_size }
	{
		if (DM_ENTRIES < (_chunk_size / _block_size)) {
			struct Dirtymap_too_small { };
			throw Dirtymap_too_small();
		}

		_size = chunk_size * _num;
		_base = (addr_t)_alloc.alloc(_size);

		for (unsigned idx = 0; idx < _num; idx++)
			_dirtymap[idx].reset();

		for (unsigned idx = 0; idx < _num; idx++) {
			_entries[idx] = { 0, 0 };
			_entries_lru[idx] = 0;
		}

		_stats.entries    = _num;
		_stats.chunk_size = _chunk_size;
		_stats.block_size = _block_size;
	}

	~Cache()
	{
		/*
		 * Be aware of flushing before destructing the cache!
		 */

		_alloc.free((void*)_base, _size);
	}

	uint32_t _count_used() const
	{
		uint32_t v = 0;
		for (unsigned i = 0; i < _num; i++) {
			v += _index_map.used(i);
		}
		return v;
	}

	uint32_t _count_dirty() const
	{
		uint32_t v = 0;
		for (unsigned i = 0; i < _num; i++) {
			v += _dirtymap[i].any_set();
		}
		return v;
	}

	Stats const &stats()
	{
		_stats.used  = _count_used();
		_stats.dirty = _count_dirty();
		return _stats;
	}

	void flush(Util::Io &io)
	{
		/*
		 * First we count all currently used entries, then we check
		 * which of these are dirty and eventually flush only the
		 * dirty ones.
		 */

		uint32_t count = 0;
		for (unsigned idx = 0; idx < _num; idx++) {
			if (_index_map.used(idx)) {
				_indices[count++] = idx;
			}
		}

		uint32_t dirty_count = 0;
		for (unsigned i = 0; i < count; i++) {
			if (_dirtymap[_indices[i]].any_set()) {
				_dirty_indices[dirty_count++] = _indices[i];
			}
		}

		for (unsigned i = 0; i < dirty_count; i++) {
			if (_flush_entry(io, _dirty_indices[i])) {
				struct Could_not_flush_entry { };
				throw Could_not_flush_entry();
			}
		}

		_stats.last_used  = count;
		_stats.last_dirty = dirty_count;
	}

	Hit fill(Util::Io &io, uint64_t lba)
	{
		if ((lba & _mask) != 0) {
			uint64_t const aligned_lba = lba + (-lba & _mask) - (_entr);
			lba = aligned_lba;
		}

		uint32_t index = ~0u;
		uint32_t lru   = ~0u;

		for (unsigned idx = 0; idx < _num; idx++) {

			/* unused always gets priority */
			if (!_index_map.used(idx)) {
				index = idx;
				break;
			}

			if (_entries_lru[idx] > lru)
				continue;

			index = idx;
			lru = _entries_lru[idx];
		}

		if (_dirtymap[index].any_set())
			_flush_entry(io, index);

		int64_t const offset = _chunk_size * index;

		using namespace Util;

		Io::Buffer iob {
			.base   = reinterpret_cast<uint8_t*>(_base + offset),
			.length = _chunk_size
		};
		Io::At const at { .value = (off_t)(lba * _block_size) };

		Io::Read_result const res = io.read(iob, at);
		if (res != Io::Read_result::READ_OK || (iob.length % _block_size))
			return Hit { 0, false };

		_entries[index].lba   = lba;
		_entries[index].count = _entr;
		_entries_lru[index]   = _lru_fn();
		_index_map.set(index);
		_stats.fills++;
		return Hit { index, true };
	}

	static bool _check_overlap(Entry const &entry, uint64_t lba)
	{
		uint64_t const start = entry.lba;
		uint64_t const end   = start + entry.count;
		return lba >= start && lba < end;
	}

	Hit hit(uint64_t lba)
	{
		for (unsigned idx = 0; idx < _num; idx++) {
			if (!_index_map.used(idx))
				continue;

			if (_check_overlap(_entries[idx], lba)) {
				_stats.hits++;
				return Hit { idx, true };
			}
		}
		_stats.misses++;
		return Hit { 0, false };
	}

	template <typename FN>
	void read_access(Hit const &hit, uint64_t lba, FN const &func)
	{
		_stats.reads++;

		int64_t const offset = _get_offset(hit.index, lba);
		func((void const * const)(_base + offset), _block_size);
		_update(hit.index, lba, false);
	}

	template <typename FN>
	void write_access(Hit const &hit, uint64_t lba, FN const &func)
	{
		_stats.writes++;

		int64_t const offset = _get_offset(hit.index, lba);
		func((void * const)(_base + offset), _block_size);
		_update(hit.index, lba, true);
	}
};


static bool perfom_blocking_io(Vfs::Env::Io                         &io,
                               Genode::Constructible<Util::Vfs_job> &job,
                               Vfs::Vfs_handle                      &handle,
                               Util::Vfs_job::Operation              op,
                               Vfs::file_offset                      offset,
                               Util::Io::Buffer                     &buffer)
{
		if (job.constructed())
			return false;

		try {
			job.construct(handle, op, offset,
			              reinterpret_cast<char*>(buffer.base),
			              buffer.length);
		} catch (...) {
			return false;
		}

		while (!job->completed())
			if (!job->execute())
				io.commit_and_wait();

		bool const succeeded = job->succeeded();
		job.destruct();

		return succeeded;
}


struct Block_io: Util::Io
{
	Vfs::Env::Io    &_io;
	Vfs::Vfs_handle &_vfs_handle;

	Genode::Constructible<Util::Vfs_job> _vfs_job { };

	Block_io(Vfs::Env::Io &io, Vfs::Vfs_handle &vfs_handle)
	:
		_io         { io },
		_vfs_handle { vfs_handle }
	{ }

	~Block_io() { }

	Write_result write(Buffer const &src, At const at) override
	{
		Util::Vfs_job::Operation const op { Util::Vfs_job::Operation::WRITE };
		bool const success = perfom_blocking_io(_io, _vfs_job, _vfs_handle,
		                                        op, at.value, const_cast<Buffer&>(src));
		return success ? Write_result::WRITE_OK
		               : Write_result::WRITE_ERR_IO;
	}

	Read_result read(Buffer &dst, At const at) override
	{
		Util::Vfs_job::Operation const op { Util::Vfs_job::Operation::READ };
		bool const success = perfom_blocking_io(_io, _vfs_job, _vfs_handle,
		                                        op, at.value, dst);
		return success ? Read_result::READ_OK
		               : Read_result::READ_ERR_IO;
	}
};


static int blockdev_io(struct ext4_blockdev *bdev,
                       void                 *buf,
                       uint64_t              lba,
                       uint32_t              count,
                       bool                  write)
{
	Lwext4::Block_device &bd = *reinterpret_cast<Lwext4::Block_device*>(bdev);

	Vfs::Env::Io    &io     =  bd._vfs_env->io();
	Vfs::Vfs_handle &handle = *bd._vfs_handle;

	size_t const block_size = bd.ext4_blockdev_iface.ph_bsize;

	if (!bd._cache) {

		Genode::Constructible<Util::Vfs_job> &job = bd._vfs_job;

		Util::Vfs_job::Operation const op { write ? Util::Vfs_job::Operation::WRITE
		                                          : Util::Vfs_job::Operation::READ };

		Vfs::file_offset const offset = block_size * lba;
		Vfs::file_size   const length = block_size * count;

		Util::Io::Buffer buffer { .base = reinterpret_cast<uint8_t*>(buf),
		                          .length = length };
		return perfom_blocking_io(io, job, handle, op, offset, buffer) ? EOK : EIO;
	}

	using Cache = Util::Cache<512>;

	Cache &cache = *bd._cache;

	Block_io bio { io, handle };

	uint64_t const start_lba = lba;

	char *data = reinterpret_cast<char*>(buf);

	for (uint32_t i = 0; i < count; i++) {

		uint64_t const lba = start_lba + i;
		uint64_t const offset = i * block_size;

		Cache::Hit hit = cache.hit(lba);
		if (!hit.valid) {
			hit = cache.fill(bio, lba);
		}
		if (!hit.valid) {
			return EIO;
		}

		if (!write) {
			cache.read_access(hit, lba,
			[&] (void const * const src, size_t length) {
				memcpy(data + offset, src, length);
			});
		} else

		if (write) {
			cache.write_access(hit, lba,
			[&] (void * const dst, size_t length) {
				memcpy(dst, data + offset , length);
			});
		}
	}

	return EOK;
}


/*
 * Lwext4 library block device interface
 */

static int blockdev_open(struct ext4_blockdev *)  { return EOK; }
static int blockdev_close(struct ext4_blockdev *) { return EOK; }


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


/*
 * VFS Lwext4 block device interface
 */

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
		vfs_env.io().commit_and_wait();

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


using Cache = Util::Cache<512>;


Lwext4::Block_device &Lwext4::block_init(Vfs::Env   &vfs_env,
                                         char const *device_path,
                                         uint32_t    block_size,
                                         size_t      cache_size)
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

	if (cache_size) {
		enum : size_t { CHUNK_SIZE = 64u << 10, };

		uint32_t const num = cache_size / CHUNK_SIZE;

		Lwext4::Cache *cache = new (vfs_env.alloc())
			Lwext4::Cache(vfs_env.alloc(), set_lru, num, CHUNK_SIZE, block_size);
		if (!cache) {
			Genode::error("could not allocate cache");
		}
		block_device->_cache = cache;
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

	if (block_device._cache)
		Genode::destroy(vfs_env.alloc(), block_device._cache);

	vfs_env.alloc().free(&block_device, sizeof (Lwext4::Block_device));
}


void Lwext4::block_sync(Block_device &block_device)
{
	if (!block_device._cache)
		return;
	
	Vfs::Env::Io    &io     =  block_device._vfs_env->io();
	Vfs::Vfs_handle &handle = *block_device._vfs_handle;

	Lwext4::Cache &cache = *block_device._cache;

	Block_io bio { io, handle };
	cache.flush(bio);
}


void Lwext4::block_cache_stats(Block_device &block_device,
                               Genode::Reporter::Xml_generator &xml)
{
	if (!block_device._cache)
		return;

	Lwext4::Cache              &cache = *block_device._cache;
	Lwext4::Cache::Stats const &stats = cache.stats();

	xml.node("block_cache", [&] () {
		xml.attribute("chunk_size", stats.chunk_size);
		xml.attribute("block_size", stats.block_size);
		xml.attribute("max_chunks", stats.entries);
		xml.node("operation", [&] () {
			xml.attribute("reads",   stats.reads);
			xml.attribute("writes",  stats.writes);
			xml.attribute("hits",    stats.hits);
			xml.attribute("misses",  stats.misses);
			xml.attribute("fills",   stats.fills);
			xml.attribute("flushes", stats.flushes);
		});
		xml.node("state", [&] () {
			xml.attribute("used",  stats.used);
			xml.attribute("dirty", stats.dirty);
			xml.attribute("last_used",  stats.last_used);
			xml.attribute("last_dirty", stats.last_dirty);
		});
	});
}

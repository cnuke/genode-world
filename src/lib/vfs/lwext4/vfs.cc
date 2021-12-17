/*
 * \brief  Lwext4 VFS plugin
 * \author Josef Soentgen
 * \date   2021-12-17
 */

/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <vfs/file_system_factory.h>
#include <vfs/file_system.h>
#include <vfs/vfs_handle.h>
#include <os/path.h>
#include <os/reporter.h>

/* library includes */
#include <lwext4/init.h>
#include <ext4.h>
#include <ext4_inode.h> /* needed for ext4_inode_get_mode() */

/* local includes */
#include "block.h"


namespace Vfs {

	using Genode::error;
	using Genode::warning;

	struct Lwext4_vfs_file_system;

};


class Vfs::Lwext4_vfs_file_system : public Vfs::File_system
{
	private:

		using Path = Genode::Path<256>;

		struct Lwext4_vfs_file_handle;
		struct Lwext4_vfs_file_watch_handle;
		struct Lwext4_vfs_dir_watch_handle;

		using Lwext4_vfs_file_handles      = Genode::List<Lwext4_vfs_file_handle>;
		using Lwext4_vfs_dir_watch_handles = Genode::List<Lwext4_vfs_dir_watch_handle>;
		using Lwext4_vfs_watch_handles     = Genode::List<Lwext4_vfs_file_watch_handle>;

		static bool wronly(int status)
		{
			using V = Vfs::Vfs_handle;
			using O = Vfs::Directory_service::Open_mode;
			auto mask = O::OPEN_MODE_ACCMODE;
			return (status & mask) == V::STATUS_WRONLY;
		}

		static bool ronly(int status)
		{
			using V = Vfs::Vfs_handle;
			using O = Vfs::Directory_service::Open_mode;
			auto mask = O::OPEN_MODE_ACCMODE;
			return (status & mask) == V::STATUS_RDONLY;
		}

		static bool rdwr(int status)
		{
			using V = Vfs::Vfs_handle;
			using O = Vfs::Directory_service::Open_mode;
			auto mask = O::OPEN_MODE_ACCMODE;
			return (status & mask) == V::STATUS_RDWR;
		}

		struct File : Genode::Avl_node<File>
		{
			Path                path;
			struct ext4_file    ext4_file;
			Lwext4_vfs_file_handles  handles;
			Lwext4_vfs_watch_handles watchers;

			bool opened() const {
				return (handles.first() || watchers.first()); }

			/************************
			 ** Avl node interface **
			 ************************/

			bool higher(File *other)
			{
				return (Genode::strcmp(other->path.base(), path.base()) > 0);
			}

			File *lookup(char const *path_str)
			{
				int const cmp = Genode::strcmp(path_str, path.base());
				if (cmp == 0)
					return this;

				File *f = Genode::Avl_node<File>::child(cmp);
				return f ? f->lookup(path_str) : nullptr;
			}
		};

		struct Lwext4_vfs_handle : Vfs_handle
		{
			using Vfs_handle::Vfs_handle;

			virtual Read_result complete_read(char *buf,
			                                  size_t buf_size,
			                                  size_t &out_count) = 0;

			virtual Write_result write(char const *buf, size_t buf_size,
			                           size_t &out_count)
			{
				return WRITE_ERR_INVALID;
			}
		};

		struct Lwext4_vfs_file_watch_handle : Vfs_watch_handle,
		                                      Lwext4_vfs_watch_handles::Element
		{
			File *file;

			Lwext4_vfs_file_watch_handle(Vfs::File_system &fs,
			                         Allocator        &alloc,
			                         File             &file)
			: Vfs_watch_handle { fs, alloc }, file { &file } { }
		};

		struct Lwext4_vfs_dir_watch_handle : Vfs_watch_handle,
		                                     Lwext4_vfs_dir_watch_handles::Element
		{
			Path const path;

			Lwext4_vfs_dir_watch_handle(Vfs::File_system &fs,
			                        Allocator        &alloc,
			                        Path       const &path)
			: Vfs_watch_handle { fs, alloc }, path { path } { }
		};

		struct Lwext4_vfs_file_handle : Lwext4_vfs_handle,
		                                Lwext4_vfs_file_handles::Element
		{
			File *file = nullptr;
			bool modifying = false;

			Lwext4_vfs_file_handle(File_system &fs,
			                   Allocator   &alloc,
			                   int          status_flags)
			: Lwext4_vfs_handle { fs, fs, alloc, status_flags } { }

			Read_result complete_read(char *buf,
			                          size_t buf_size,
			                          size_t &out_count) override
			{
				if (!file)
					return READ_ERR_INVALID;

				if (wronly(status_flags()))
					return READ_ERR_INVALID;

				int err = ext4_fseek(&file->ext4_file, seek(), SEEK_SET);
				if (err) // TODO check err
					return READ_ERR_IO;

				size_t read = 0;
				err = ext4_fread(&file->ext4_file, buf, buf_size, &read);
				if (err) // TODO check err
					return READ_ERR_IO;

				out_count = (file_size)read;

				return READ_OK;
			}

			Write_result write(char const *buf, size_t buf_size,
			                   size_t &out_count) override
			{
				if (!file)
					return WRITE_ERR_INVALID;
				if (ronly(status_flags()))
					return WRITE_ERR_INVALID;

				int res = 0;
				ext4_file *fil = &file->ext4_file;
				file_size const wpos = seek();

				/* seek file pointer */
				if (ext4_ftell(fil) != wpos) {
					/*
					 * Seeking beyond the EOF will expand the file size
					 * and is not supported.
					 */
					if (ext4_fsize(fil) < wpos)
						return WRITE_ERR_INVALID;

					res = ext4_fseek(fil, wpos, SEEK_SET);
					/* check the seek again */
					if (ext4_ftell(fil) != seek())
						return WRITE_ERR_IO;
				}

				if (!res) {
					size_t written = 0;
					res = ext4_fwrite(fil, buf, buf_size, &written);
					if (res)
						return WRITE_ERR_IO;

					modifying = true;
					out_count = (file_size)written;
					return WRITE_OK;
				}

				return WRITE_ERR_IO;
			}
		};

		struct Lwext4_vfs_dir_handle;

		struct Lwext4_vfs_dir_handle : Lwext4_vfs_handle
		{
			Path const path;
			ext4_dir dir;

			Lwext4_vfs_dir_handle(File_system &fs, Allocator &alloc, char const *path)
			: Lwext4_vfs_handle(fs, fs, alloc, 0), path(path) { }

			Read_result complete_read(char      *buf,
			                          size_t  buf_size,
			                          size_t &out_count) override
			{
				out_count = 0;

				if (buf_size < sizeof (Dirent))
					return READ_ERR_INVALID;

				file_size const seek_offset = seek();
				if (seek_offset % sizeof (Dirent))
					return READ_ERR_INVALID;

				file_size dir_index = seek_offset / sizeof (Dirent);


				/*
				 * Manipulate ext4_dir struct directly which AFAICT is
				 * okay and let lwext4 deal with it.
				 *
				 * Reset the next offset when the first entry should
				 * be read as a user may seek arbitrarily.
				 */
				if (dir_index == 0)
					dir.next_off = 0;

				Dirent &vfs_dirent = *(Dirent*)buf;

				ext4_direntry const *dentry = nullptr;
				while (true) {
					dentry = ext4_dir_entry_next(&dir);
					if (!dentry) { break; }

					/* ignore entries without proper inode */
					if (!dentry->inode)
						continue;

					/*
					 * Ignore dot entries here as lwext4 delivers
					 * them to us.
					 */
					if (dentry->name_length == 1)
						if (dentry->name[0] == '.')
							continue;
					if (dentry->name_length == 2)
						if (dentry->name[0] == '.' && dentry->name[1] == '.')
							continue;

					break;
				}

				if (!dentry) {

					vfs_dirent = {
						.fileno = 1,
						.type   = Dirent_type::END,
						.rwx    = Node_rwx::rwx(),
						.name   = { }
					};
					out_count = sizeof(Dirent);
					return READ_OK;
				}

				Dirent_type type = Dirent_type::END;
				switch (dentry->inode_type) {
				case EXT4_DE_DIR:     type = Dirent_type::DIRECTORY;       break;
				case EXT4_DE_SYMLINK: type = Dirent_type::SYMLINK;         break;
				default:              type = Dirent_type::CONTINUOUS_FILE; break;
				}
				vfs_dirent = {
					.fileno = dentry->inode,
					.type   = type,
					.rwx    = Node_rwx::rwx(),
					.name   = { }
				};

				size_t const len = (size_t)(dentry->name_length + 1) > sizeof(vfs_dirent.name.buf)
				                 ? sizeof (vfs_dirent.name.buf) : dentry->name_length + 1;
				copy_cstring(vfs_dirent.name.buf,
				             reinterpret_cast<char const*>(dentry->name), len);

				out_count = sizeof (Dirent);
				return READ_OK;
			}
		};

		class Lwext4_vfs_symlink_handle : public Lwext4_vfs_handle
		{
			private:

				Path const _path;

			public:

				Lwext4_vfs_symlink_handle(File_system &fs, Allocator &alloc,
				                        int status_flags, char const *path)
				: Lwext4_vfs_handle(fs, fs, alloc, status_flags), _path(path) { }

				Read_result complete_read(char *buf, size_t buf_size,
				                          size_t &out_count) override
				{
					/* partial read is not supported */
					if (seek() != 0) {
						return READ_ERR_INVALID;
					}

					size_t bytes;
					if (ext4_readlink(_path.base(), buf, buf_size, &bytes))
						return READ_ERR_IO;

					out_count = bytes;
					return READ_OK;
				}

				Write_result write(char const *buf, size_t buf_size,
				                   size_t &out_count) override
				{
					/* partial write is not supported */
					if (seek() != 0) {
						return WRITE_ERR_INVALID;
					}

					// TODO check path truncation
					Genode::String<MAX_PATH_LEN> const target {
						Genode::Cstring {buf, buf_size }
					};

					if (ext4_fsymlink(target.string(), _path.base()) != 0) {
						out_count = 0;
						return WRITE_OK;
					}

					out_count = buf_size;
					return WRITE_OK;
				}
		};

		Vfs::Env &_vfs_env;

		/* List of all open directory handles */
		Lwext4_vfs_dir_watch_handles _dir_watchers;

		/* Tree of open Lwext4 file objects */
		Genode::Avl_tree<File> _open_files;

		/* Pre-allocated FIL */
		File *_next_file = nullptr;

		/**
		 * Return an open Lwext4 file matching path or null.
		 */
		File *_opened_file(char const *path)
		{
			return _open_files.first() ?
				_open_files.first()->lookup(path) : nullptr;
		}

		/**
		 * Notify the application for each handle on a given file.
		 */
		void _notify(File &file)
		{
			for (Lwext4_vfs_file_watch_handle *h = file.watchers.first(); h; h = h->next())
				h->watch_response();
		}

		/**
		 * Notify the application for each handle on the parent
		 * directory of a given path.
		 */
		void _notify_parent_of(char const *path)
		{
			Path parent(path);
			parent.strip_last_element();

			for (Lwext4_vfs_dir_watch_handle *h = _dir_watchers.first(); h; h = h->next())
				if (h->path == parent)
					h->watch_response();
		}

		/**
		 * Close an open Lwext4 file
		 */
		void _close(File &file)
		{
			/* close file */
			_open_files.remove(&file);
			ext4_fclose(&file.ext4_file);

			if (_next_file == nullptr) {
				/* reclaim heap space */
				file.path.import("");
				_next_file = &file;
			} else {
				destroy(_vfs_env.alloc(), &file);
			}
		}

		/**
		 * Invalidate all handles on a Lwext4 file
		 * and close the file
		 */
		void _close_all(File &file)
		{
			/* invalidate handles */
			for (auto *handle = file.handles.first();
			     handle; handle = file.handles.first())
			{
				handle->file = nullptr;
				file.handles.remove(handle);
			}

			for (auto *handle = file.watchers.first();
			     handle; handle = file.watchers.first())
			{
				handle->file = nullptr;
				file.watchers.remove(handle);
				handle->watch_response();
			}
			_close(file);
		}

		char const *_fs_name = "ext4";
		char const *_fs_mp   = "/";
		bool        _cache_write_back = false;

		bool _io_truncation;
		char _truncate_buf[64u << 10] { };

		Lwext4::Block_device &_block_device;

		Genode::Constructible<Genode::Reporter> _stats_reporter { };

		bool _report_cache;

		void _report_stats(Genode::Reporter &reporter, bool report_cache)
		{
			using namespace Genode;

			struct ext4_mount_stats stats { };
			int const err = ext4_mount_point_stats(_fs_mp, &stats);
			if (err) {
				Genode::error("could not get mount point stats");
				return;
			}

			try {
				Reporter::Xml_generator xml(reporter, [&] () {
					xml.node("space", [&] () {
						Util::Number_of_bytes const total =
							stats.blocks_count * stats.block_size;
						Util::Number_of_bytes const avail =
							stats.free_blocks_count * stats.block_size;
						xml.attribute("total", Genode::String<16>(total));
						xml.attribute("avail", Genode::String<16>(avail));
					});
					xml.node("blocks", [&] () {
						xml.attribute("total", stats.blocks_count);
						xml.attribute("avail", stats.free_blocks_count);
						xml.attribute("size",  stats.block_size);
					});
					xml.node("inodes", [&] () {
						xml.attribute("total",  stats.inodes_count);
						xml.attribute("avail", stats.free_inodes_count);
					});

					if (report_cache)
						Lwext4::block_cache_stats(_block_device, xml);
				});
			} catch (...) { }
		}

	public:

		struct Init_failed    : Genode::Exception { };
		struct Mount_failed   : Genode::Exception { };
		struct Unmount_failed : Genode::Exception { };

		Lwext4_vfs_file_system(Vfs::Env             &env,
		                       Genode::Xml_node      config,
		                       Lwext4::Block_device &block_device,
		                       bool                  io_truncation,
		                       bool                  reporting,
		                       bool                  report_cache)
		:
			_vfs_env { env },
			_io_truncation { io_truncation },
			_block_device  { block_device },
			_report_cache  { report_cache }
		{
			// TODO put into proper location
			Lwext4::malloc_init(_vfs_env.env(), _vfs_env.alloc());

			ext4_blockdev *bd = &_block_device.ext4_blockdev;
			int err = ext4_device_register(bd, _fs_name);
			if (err)
				throw Init_failed();

			bool const writeable = config.attribute_value("writeable", false);
			err = ext4_mount(_fs_name, _fs_mp, !writeable);
			if (err) {
				error("could not mount file system, err: ", err);
				throw Mount_failed();
			}

			_cache_write_back = config.attribute_value("cache_write_back", false);
			if (_cache_write_back) {
				err = ext4_cache_write_back(_fs_mp, 1);
				if (err) {
					warning("could not enable cache write-back mode, err: ", err);
					_cache_write_back = false;
				}
			}

			err = ext4_recover(_fs_mp);
			if (err && err != ENOTSUP) {
				error("could not recover file system (FSCK needed!), err:", err);
				throw Mount_failed();
			}

			err = ext4_journal_start(_fs_mp);
			if (err) {
				error("could not start journal, err: ", err);
				throw Mount_failed();
			}

			try {
				if (reporting) {
					_stats_reporter.construct(_vfs_env.env(), "stats", "filesystem_stats");
					_stats_reporter->enabled(true);

					/* create initial stats report */
					_report_stats(*_stats_reporter, _report_cache);
				}
			} catch (...) {
				Genode::warning("could not enable stats reporting");
			}
		}

		~Lwext4_vfs_file_system()
		{
			int err = ext4_journal_stop(_fs_mp);
			if (err)
				warning("could not stop journal, err: ", err);

			if (_cache_write_back) {
				err = ext4_cache_write_back(_fs_mp, 0);
				if (err)
					warning("could not disable cache write-back mode, err: ", err);
			}

			err = ext4_umount(_fs_mp);
			if (err)
				error("could not unmount file system, err: ", err);

			Lwext4::block_sync(_block_device);
		}

		/***************************
		 ** File_system interface **
		 ***************************/

		char const *type() override { return "lwext4"; }


		/*********************************
		 ** Directory service interface **
		 *********************************/

		Open_result open(char const *path, unsigned vfs_mode,
		                 Vfs_handle **vfs_handle,
		                 Allocator  &alloc) override
		{
			Lwext4_vfs_file_handle *handle;

			File *file = _opened_file(path);

			bool create = vfs_mode & OPEN_MODE_CREATE;

			if (file && create) {
				return OPEN_ERR_EXISTS;
			}

			/* attempt allocation before modifying blocks */
			if (!_next_file)
				_next_file = new (_vfs_env.alloc()) File();

			handle = new (alloc) Lwext4_vfs_file_handle(*this, alloc, vfs_mode);

			if (!file) {

				file = _next_file;

				int flags = 0;
				if (create) { flags |= O_CREAT; }

				// using O = Vfs::Directory_service::Open_mode;
				// switch (vfs_mode & O::OPEN_MODE_ACCMODE) {
				// case O::OPEN_MODE_RDONLY: flags |= O_RDONLY; break;
				// case O::OPEN_MODE_WRONLY: flags |= O_WRONLY; break;
				// case O::OPEN_MODE_RDWR:   flags |= O_RDWR;   break;
				// default: break;
				// }

				/*
				 * Since a File object can by used from a WRONLY as
				 * well as RDONLY Vfs_handle concurrently we always
				 * open the file internally as r/w and let the specific
				 * Lwext4_vfs_file_handle enforce the mode.
				 */
				flags |= O_RDWR;

				using OR = Vfs::Directory_service::Open_result;
				OR result = OR::OPEN_ERR_UNACCESSIBLE;
				do {
					if (ext4_fopen2(&file->ext4_file, path, flags))
						break;

					if (create && ext4_mode_set(path, 0666))
						break;

					result = OR::OPEN_OK;
				} while (false);

				if (result != OR::OPEN_OK) {
					destroy(alloc, handle);
					return result;
				}

				file->path.import(path);
				_open_files.insert(file);
				_next_file = nullptr;
			}

			if (create)
				_notify_parent_of(path);

			file->handles.insert(handle);
			handle->file = file;
			*vfs_handle = handle;
			return OPEN_OK;
		}

		Opendir_result opendir(char const *path, bool create,
		                       Vfs_handle **vfs_handle,
		                       Allocator &alloc) override
		{
			/* attempt allocation before modifying blocks */
			Lwext4_vfs_dir_handle *handle =
				new (alloc) Lwext4_vfs_dir_handle(*this, alloc, path);

			Opendir_result result = OPENDIR_OK;
			do {
				/* always try to open the directory first */
				int err = ext4_dir_open(&handle->dir, path);

				if (!err && !create) {
					result = OPENDIR_OK;
					break;
				} else

				if (err && !create) {
					result = OPENDIR_ERR_PERMISSION_DENIED;
					break;
				} else

				if (!err && create) {
					ext4_dir_close(&handle->dir);
					result = OPENDIR_ERR_NODE_ALREADY_EXISTS;
					break;
				} else

				if (err && create) {
					err = ext4_dir_mk(path);
					if (err) { // TODO check err
						result = OPENDIR_ERR_PERMISSION_DENIED;
						break;
					}

					err = ext4_mode_set(path, 0777);
					if (err) { // TODO check err
						ext4_dir_rm(path);
						result = OPENDIR_ERR_PERMISSION_DENIED;
						break;
					}

					/* try again after mkdir */
					create = false;
					continue;
				}
			} while (true);

			if (result != OPENDIR_OK) {
				destroy(alloc, handle);
				return result;
			}

			*vfs_handle = handle;
			return OPENDIR_OK;
		}

		Openlink_result openlink(char const *path, bool create,
		                         Vfs_handle **handle, Allocator &alloc) override
		{
			if (create)
				if (ext4_fsymlink("", path) != 0) // TODO check error
					return OPENLINK_ERR_PERMISSION_DENIED;

			size_t bytes;
			char dummy;
			if (ext4_readlink(path, &dummy, sizeof(dummy), &bytes))
				return OPENLINK_ERR_PERMISSION_DENIED;

			try {
				*handle = new (alloc) Lwext4_vfs_symlink_handle(*this, alloc, 0777, path);
				return OPENLINK_OK;
			}
			catch (Genode::Out_of_ram) { return OPENLINK_ERR_OUT_OF_RAM; }
			catch (Genode::Out_of_caps) { return OPENLINK_ERR_OUT_OF_CAPS; }
		}

		void close(Vfs_handle *vfs_handle) override
		{
			{
				auto *handle = dynamic_cast<Lwext4_vfs_file_handle *>(vfs_handle);
				bool notify = false;

				if (handle) {
					File *file = handle->file;
					if (file) {
						file->handles.remove(handle);
						if (file->opened()) {
							notify = handle->modifying;
						} else {
							_close(*file);
							file = nullptr;
						}
					}
					destroy(handle->alloc(), handle);

					if (notify && file)
						_notify(*file);
					return;
				}
			}

			{
				auto *handle = dynamic_cast<Lwext4_vfs_dir_handle *>(vfs_handle);

				if (handle) {
					ext4_dir_close(&handle->dir);
					destroy(handle->alloc(), handle);
				}
			}

			{
				auto *handle = dynamic_cast<Lwext4_vfs_symlink_handle *>(vfs_handle);

				if (handle) {
					destroy(handle->alloc(), handle);
				}
			}
		}

		Watch_result watch(char const      *path,
		                   Vfs_watch_handle **handle,
		                   Allocator        &alloc) override
		{
			/*
			 * checking for the presence of an open file is
			 * cheaper than calling directory and reading blocks
			 */
			File *file = _opened_file(path);

			if (!file && directory(path)) {
				auto *watch_handle = new (alloc)
					Lwext4_vfs_dir_watch_handle(*this, alloc, path);
				_dir_watchers.insert(watch_handle);
				*handle = watch_handle;
				return WATCH_OK;
			} else {
				if (!file) {
					if (!_next_file)
						_next_file = new (_vfs_env.alloc()) File();

					file = _next_file;
					if (ext4_fopen2(&file->ext4_file, path, O_RDONLY)) {
						return WATCH_ERR_UNACCESSIBLE;
					}

					file->path.import(path);
					_open_files.insert(file);
					_next_file = nullptr;
				}

				auto *watch_handle = new (alloc)
					Lwext4_vfs_file_watch_handle(*this, alloc, *file);
				file->watchers.insert(watch_handle);
				*handle = watch_handle;
				return WATCH_OK;
			}
			return WATCH_ERR_UNACCESSIBLE;
		}

		void close(Vfs_watch_handle *vfs_handle) override
		{
			{
				auto *handle =
					dynamic_cast<Lwext4_vfs_file_watch_handle *>(vfs_handle);

				if (handle) {
					File *file = handle->file;
					if (file)
						file->watchers.remove(handle);
					destroy(handle->alloc(), handle);
					return;
				}
			}

			{
				auto *handle =
					dynamic_cast<Lwext4_vfs_dir_watch_handle *>(vfs_handle);

				if (handle) {
					_dir_watchers.remove(handle);
					destroy(handle->alloc(), handle);
				}
			}
		}


		Genode::Dataspace_capability dataspace(char const *path) override
		{
			Genode::warning(__func__, " not implemented in Lwext4 plugin");
			return Genode::Dataspace_capability();
		}

		void release(char const *path,
		             Genode::Dataspace_capability ds_cap) override { }

		file_size num_dirent(char const *path) override
		{
			ext4_dir dir;

			int err = ext4_dir_open(&dir, path);
			if (err) // TODO check error
				return 0;

			file_size count = 0;

			ext4_direntry const *dentry = nullptr;
			while (true) {
				dentry = ext4_dir_entry_next(&dir);
				if (!dentry)
					break;

				if (!dentry->inode)
					continue;

				++count;
			}

			ext4_dir_close(&dir);
			return count;
		}

		Node_type _node_type(char const *path)
		{
			/* use TRANSACTIONAL_FILE as invalid indicator */
			Node_type result { Node_type::TRANSACTIONAL_FILE };

			struct ext4_inode _inode;
			unsigned int      _ino;

			int err = ext4_raw_inode_fill(path, &_ino, &_inode);
			if (err)
				return result;

			struct ext4_sblock *sb;
			err = ext4_get_sblock(path, &sb);
			if (err)
				return result;

			unsigned int const mode = ext4_inode_get_mode(sb, &_inode);
		 	unsigned int const type = mode & 0xf000;

			switch (type) {
			case EXT4_INODE_MODE_DIRECTORY: result = Node_type::DIRECTORY; break;
			case EXT4_INODE_MODE_SOFTLINK:  result = Node_type::SYMLINK;   break;
			case EXT4_INODE_MODE_FILE:
			default:                        result = Node_type::CONTINUOUS_FILE; break;
			}
			return result;
		}

		bool directory(char const *path) override
		{
			if (path[0] == '/' && path[1] == '\0') return true;

			return _node_type(path) == Node_type::DIRECTORY;
		}

		char const *leaf_path(char const *path) override
		{
			if (_opened_file(path))
				return path;

			return _node_type(path) != Node_type::TRANSACTIONAL_FILE ? path : 0;
		}

		Stat_result stat(char const *path, Stat &stat) override
		{
			stat = Stat { };

			struct ext4_inode _inode;
			unsigned int      _ino;

			int err = ext4_raw_inode_fill(path, &_ino, &_inode);
			if (err)
				return STAT_ERR_NO_ENTRY;

			struct ext4_sblock *sb;
			err = ext4_get_sblock(path, &sb);
			if (err)
				return STAT_ERR_NO_ENTRY;

			stat.size   = ext4_inode_get_size(sb, &_inode);
			stat.inode  = (unsigned long)_ino;
			stat.device = (Genode::addr_t)this;

			unsigned int mtime = 0;
			(void) ext4_mtime_get(path, &mtime);
			stat.modification_time = { mtime };

			unsigned int const mode = ext4_inode_get_mode(sb, &_inode);
		 	unsigned int const type = mode & 0xf000;
			switch (type) {
			case EXT4_INODE_MODE_DIRECTORY: stat.type = Node_type::DIRECTORY; break;
			case EXT4_INODE_MODE_SOFTLINK:  stat.type = Node_type::SYMLINK;   break;
			case EXT4_INODE_MODE_FILE:
			default:                        stat.type = Node_type::CONTINUOUS_FILE; break;
			}

			stat.rwx = { .readable   = !!(mode & 0x100),
			             .writeable  = !!(mode & 0x080),
			             .executable = !!(mode & 0x040),
			};

			return STAT_OK;
		}

		Unlink_result unlink(char const *path) override
		{
			/* close the file if it is open */
			if (File *file = _opened_file(path)) {
				_notify(*file);
				_close_all(*file);
			}

			{
				struct ext4_inode _inode;
				unsigned int      _ino;
				int err = ext4_raw_inode_fill(path, &_ino, &_inode);
				/* silent error because the look up is allowed to fail */
				if (err)
					return UNLINK_ERR_NO_ENTRY;

				unsigned int const v = _inode.mode & 0xf000;
				switch (v) {
				case EXT4_INODE_MODE_DIRECTORY:
					err = ext4_dir_rm(path);
					break;
				case EXT4_INODE_MODE_FILE: [[fallthrough]];
				case EXT4_INODE_MODE_SOFTLINK:
				default:
					err = ext4_fremove(path);
					break;
				}
				if (err) // TODO check error
					return UNLINK_ERR_NO_ENTRY;
			}

			_notify_parent_of(path);
			return UNLINK_OK;
		}

		Rename_result rename(char const *from, char const *to) override
		{
			if (File *to_file = _opened_file(to)) {
				_notify(*to_file);
				_close_all(*to_file);
			} else
				if (_node_type(to) == Node_type::DIRECTORY)
					return RENAME_ERR_NO_PERM;

			/* remove target if it already exists to prevent EEXIST*/
			(void)unlink(to);

			if (File *from_file = _opened_file(from)) {
				_notify(*from_file);
				_close_all(*from_file);
			}

			int const err = ext4_frename(from, to);
			if (err) // TODO check err
				return RENAME_ERR_NO_ENTRY;

			_notify_parent_of(from);
			if (Genode::strcmp(from, to) != 0)
				_notify_parent_of(to);
			return RENAME_OK;
		}


		/*******************************
		 ** File io service interface **
		 *******************************/

		Write_result write(Vfs_handle *vfs_handle,
		                   Const_byte_range_ptr const &src,
		                   size_t &out_count) override
		{
			auto *handle = dynamic_cast<Lwext4_vfs_handle *>(vfs_handle);

			if (handle)
				return handle->write(src.start, src.num_bytes, out_count);

			return WRITE_ERR_INVALID;
		}

		Read_result complete_read(Vfs_handle *vfs_handle,
		                          Byte_range_ptr const &dst,
		                          size_t &out_count) override
		{
			auto *handle = dynamic_cast<Lwext4_vfs_handle *>(vfs_handle);

			if (handle)
				return handle->complete_read(dst.start, dst.num_bytes, out_count);

			return READ_ERR_INVALID;
		}

		Ftruncate_result ftruncate(Vfs_handle *vfs_handle, file_size len) override
		{
			Lwext4_vfs_file_handle *handle = dynamic_cast<Lwext4_vfs_file_handle *>(vfs_handle);

			if (!handle)
				return FTRUNCATE_ERR_NO_PERM;

			if (!handle->file)
				return FTRUNCATE_ERR_NO_PERM;

			if (ronly(handle->status_flags()))
				return FTRUNCATE_ERR_NO_PERM;

			ext4_file *fil = &handle->file->ext4_file;

			uint64_t const fsize = ext4_fsize(fil);
			if (fsize < len) {
				if (!_io_truncation)
					return FTRUNCATE_ERR_NO_PERM;
				else {
					size_t const amount = len - fsize;

					for (size_t increase = amount; increase && increase <= amount; ) {
						size_t out_count;

						size_t const current_size =
							min(increase, sizeof (_truncate_buf));

						Write_result const res =
							handle->write(_truncate_buf, current_size, out_count);
						if (res != WRITE_OK || out_count != current_size)
							return FTRUNCATE_ERR_NO_PERM;

						increase -= current_size;
					}
				}
			}

			int res = ext4_ftruncate(fil, len);
			if (res)
				return FTRUNCATE_ERR_NO_PERM;

			if (len < handle->seek())
				handle->seek(len);

			handle->modifying = true;
			return FTRUNCATE_OK;
		}

		bool read_ready(Vfs_handle const &) const override { return true; }

		bool write_ready(Vfs_handle const &) const override
		{
			/* wakeup from WRITE_ERR_WOULD_BLOCK not supported */
			return true;
		}

		/**
		 * Notify other handles if this handle has modified its file.
		 */
		Sync_result complete_sync(Vfs_handle *vfs_handle) override
		{
			int err = ext4_cache_flush(_fs_mp);
			Sync_result result = err ? SYNC_ERR_INVALID : SYNC_OK;

			if (_stats_reporter.constructed() && _stats_reporter->enabled())
				_report_stats(*_stats_reporter, _report_cache);

			Lwext4::block_sync(_block_device);

			Lwext4_vfs_file_handle *handle =
				dynamic_cast<Lwext4_vfs_file_handle*>(vfs_handle);

			/* symlinks end up here */
			if (!handle)
				return result;

			if (handle && handle->file && handle->modifying) {
				File &file = *handle->file;
				handle->modifying = false;
				file.handles.remove(handle);
				_notify(file);
				file.handles.insert(handle);
			}

			return result;
		}

		bool update_modification_timestamp(Vfs_handle *vfs_handle,
		                                   Vfs::Timestamp ts)
		{
			Lwext4_vfs_file_handle *handle =
				dynamic_cast<Lwext4_vfs_file_handle*>(vfs_handle);
			if (handle && handle->file) {
				/* lwext4 only supports 32bit time values */
				uint32_t const mtime = ts.value > 0 ? (uint32_t)ts.value : 0;

				/* silently ignore errors */
				(void)ext4_mtime_set(handle->file->path.base(), mtime);
			}

			return true;
		}
};


struct Lwext4_factory : Vfs::File_system_factory
{
	Vfs::File_system *create(Vfs::Env &vfs_env, Genode::Xml_node node) override
	{
		using Path = Genode::String<256>;
		Path const block_device = node.attribute_value("block_device",
		                                               Path("/dev/block"));
		uint32_t const block_size = node.attribute_value("block_size", 512U);
		bool const io_truncation = node.attribute_value("expand_via_io", false);
		Genode::Number_of_bytes const cache_size =
			node.attribute_value("external_cache_size", Genode::Number_of_bytes(0));
		bool const reporting    = node.attribute_value("reporting", false);
		bool const report_cache = node.attribute_value("report_cache", false);

		Lwext4::Block_device *bp = nullptr;
		try {
			Lwext4::Block_device &bd =
				Lwext4::block_init(vfs_env, block_device.string(),
				                   block_size, cache_size);
			bp = &bd;
		} catch (...) {
			Genode::error("could not initialize lwext4 block back end");
			return nullptr;
		}

		try {
			return new (vfs_env.alloc())
				Vfs::Lwext4_vfs_file_system(vfs_env, node, *bp, io_truncation,
				                            reporting, report_cache);
		} catch (...) {
			Genode::error("could not create lwext4 filesystem");
			Lwext4::block_deinit(vfs_env, *bp);
		}
		return nullptr;
	}
};


extern "C" Vfs::File_system_factory *vfs_file_system_factory(void)
{
	static Lwext4_factory factory;
	return &factory;
}

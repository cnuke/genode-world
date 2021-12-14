/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <os/path.h>
#include <os/vfs.h>  // Directory::Path
#include <libc/component.h>
#include <vfs/file_system_factory.h>
#include <vfs/file_system.h>
#include <vfs/vfs_handle.h>

/* libc includes */
#include <sys/dirent.h>
#include <errno.h>

/* FUSe includes */
#include <fuse.h>
#include <fuse_private.h>

/* local includes */
#if 0
#include "block_dev.h"
#else
#include "blocks_as_file.h"
#endif


#if 0
#define TRACE(x, ...) Genode::log( "---->", x, ##__VA_ARGS__ )
#else
#define TRACE(x, ...)
#endif

namespace {

	using namespace Vfs;

};


#if 0
Blockdev * ptr_to_blockdev = NULL;
#else
Blocks_as_file * ptr_to_blockdev = NULL;
#endif


class Fuse_file_system/*Fuse___for_ntfs__file_system*/ : public Vfs::File_system
{
	private:

		struct Dir_vfs_handle : Vfs::Vfs_handle//Vfs::Single_vfs_handle
		{
			Dir_vfs_handle(Directory_service &ds,
			                File_io_service   &fs,
			                Genode::Allocator &alloc)
			: Vfs_handle(ds, fs, alloc, 0) { }
		};
		
		struct File_vfs_handle : Vfs::Vfs_handle//Vfs::Single_vfs_handle
		{
			//Directory::Path _path;//or in a (common/parent) class?
			String<256> _path;
			struct fuse_file_info  _file_info;
			/**/

			File_vfs_handle(Directory_service &ds,
			                File_io_service   &fs,
			                Genode::Allocator &alloc,
			                Directory::Path path)
			: Vfs_handle(ds, fs, alloc, 0),
			_path(path),
			_file_info({})
			{
/////FIXME !! it barely works as-is...
				int res = Fuse::fuse()->op.open(path.string(), &_file_info);
if (res != 0) Genode::warning("could not open path for Vfs Handle");
				mode_t mode = S_IFREG | 0644;
				res = Fuse::fuse()->op.mknod(path.string(), mode, 0);
				if (res != 0)
					Genode::warning("could not create '", path, "'");
		//+		res = Fuse::fuse()->op.ftruncate(path, 0, &_file_info);
				///
			}

			Read_result read(char *dest, file_size len, file_size &out_count) //override
			{
				out_count = 0;

				/* current offset */
				file_size seek_offset = seek();
///later: seems to work without adjusted append/seek offset, but is it safe ?
//				if (seek_offset == ~0ULL)
//					seek_offset = _length();

				int ret = Fuse::fuse()->op.read(_path.string(), dest, len,
				                                seek_offset, &_file_info);

				if (ret < 0) return READ_ERR_INVALID;

				out_count = ret;
				return READ_OK;
			}

			Write_result write(char const *src, file_size len,
			                   file_size &out_count) //override
			{
				out_count = 0;

				/* current read offset */
				file_size const seek_offset = seek();

				int ret = Fuse::fuse()->op.write(_path.string()/*.base()*/, src, len,
				                                 seek_offset, &_file_info);

				if (ret < 0) return WRITE_ERR_INVALID;//

				out_count = ret;
				return WRITE_OK;
			}

			Ftruncate_result ftruncate (file_size len)
			{
				int ret = Fuse::fuse()->op.ftruncate(_path.string(), len,
				                                     &_file_info);
				if (ret < 0) return FTRUNCATE_ERR_NO_PERM;//

				return FTRUNCATE_OK;
			}

		//	bool read_ready() override { return false; }
		};
		
	private:
		/*
		todo: add book-keeping stuff (?):
		
		class Fuse_vfs_file : public Genode::List<Fuse_vfs_file>::Element
			or : 		struct File : Genode::Avl_node<File>
		class Fuse_vfs_handle : public Vfs::Vfs_handle
			struct Fuse_file_watch_handle : Vfs_watch_handle,  Fuse_watch_handles::Element
			struct Fuse_dir_watch_handle : Vfs_watch_handle, Fuse_dir_watch_handles::Element
		class Fuse_vfs_file_handle : public Fuse_vfs_handle
			or : Fatfs_handle, Fatfs_file_handles::Element
		class Fuse_vfs_dir_handle : public Fuse_vfs_handle
		*/

		/*
		FUSe fuse....;
		
		Genode::List<Dataspace_vfs_file> _files { };
		..
			or :
		_open_files..
		//_dir_watchers..
		..
		*/
		
		Vfs::Env &_vfs_env;
		
#if 0
		Blockdev _block { _vfs_env.env(), _vfs_env.alloc() };
#else
		Blocks_as_file _block { _vfs_env };
#endif

	public:

		Fuse_file_system(Vfs::Env &env, Genode::Xml_node config)
		: _vfs_env(env)
		{
#if 0
#else
			// e.g. "/dev/sda1"
			Genode::Directory::Path path(config.attribute_value("block_path", Genode::Directory::Path()));
#endif

			/* mount the file system */
			
			///later: find a way to embed this pointer inside NTFS instead, to be cleaner
			ptr_to_blockdev = &_block;
extern void malloc_init(Genode::Env &env, Genode::Allocator &alloc);//
			malloc_init(_vfs_env.env(), _vfs_env.alloc());

			Genode::log( "-- Starting fuse with device file '", path, "' --" );

			//if (Fuse::initialized() == 0 ) {
				if (!Fuse::init_fs(path.string())) {
					Genode::error("FUSE vfs initialization failed");
			//		return;
				}
			//}
		}

		~Fuse_file_system()
		{
			if (Fuse::initialized()) {
				Fuse::deinit_fs();
			}
		}

		/***************************
		 ** File_system interface **
		 ***************************/

		///? static char const *name() { return "fuse_ntfs3g__XX"; }
		char const *type() override { return "fuse_ntfs3g__XX"; }

		/*********************************
		 ** Directory service interface **
		 *********************************/

		Open_result open(char const *path, unsigned vfs_mode,
		                 Vfs_handle **handle,
		                 Allocator  &alloc) override
		{
			TRACE( "open path=", path, " mode=", vfs_mode );

/*
			Dataspace_vfs_file *file { };

			bool const create = mode & OPEN_MODE_CREATE;

			if (create) {

				if (_lookup(path))
					return OPEN_ERR_EXISTS;

				if (strlen(path) >= MAX_NAME_LEN)
					return OPEN_ERR_NAME_TOO_LONG;

				try { file = new (alloc) Dataspace_vfs_file(path, alloc, _env.env().ram()); }
				catch (Allocator::Out_of_memory) { return OPEN_ERR_NO_SPACE; }

				_files.insert(file);
			} else {
				file = _lookup(path);
				if (!file) return OPEN_ERR_UNACCESSIBLE;
			}
*/

			try {
				*handle = new (alloc) File_vfs_handle(*this, *this, alloc, path);//, file);
			}
			catch (Genode::Out_of_ram)  { return OPEN_ERR_OUT_OF_RAM; }
			catch (Genode::Out_of_caps) { return OPEN_ERR_OUT_OF_CAPS; }

			return OPEN_OK;
		}

		Opendir_result opendir(char const *path, bool create,
		                       Vfs_handle **handle,
		                       Allocator &alloc) override
		{
			TRACE( "opendir path=", path, " create=", create );

			if (create) {
				int res = Fuse::fuse()->op.mkdir(path, 0755);

				if (res < 0) {
					int err = -res;
					switch (err) {
						case EACCES:
							Genode::error("op.mkdir() permission denied");
							return OPENDIR_ERR_PERMISSION_DENIED;
						case EEXIST:
							return OPENDIR_ERR_NODE_ALREADY_EXISTS;
						case EIO:
							Genode::error("op.mkdir() I/O error occurred");
							return OPENDIR_ERR_LOOKUP_FAILED;
						case ENOENT:
							return OPENDIR_ERR_LOOKUP_FAILED;
						case ENOTDIR:
							return OPENDIR_ERR_LOOKUP_FAILED;
						case ENOSPC:
							Genode::error("op.mkdir() error while expanding directory");
							return OPENDIR_ERR_LOOKUP_FAILED;
						case EROFS:
							return OPENDIR_ERR_PERMISSION_DENIED;
						default:
							Genode::error("op.mkdir() returned unexpected error code: ", res);
							return OPENDIR_ERR_LOOKUP_FAILED;
					}
				}
			}

//
			struct fuse_file_info  _file_info;///
			int res = Fuse::fuse()->op.opendir(path, &_file_info);
///FIXME: release(): where do we do the matching
//				Fuse::fuse()->op.release(_path.base(), &_file_info);
// ?

			if (res < 0) {
				int err = -res;
				switch (err) {
					case EACCES:
						Genode::error("op.opendir() permission denied");
						return OPENDIR_ERR_PERMISSION_DENIED;
					case EIO:
						Genode::error("op.opendir() I/O error occurred");
						return OPENDIR_ERR_LOOKUP_FAILED;
					case ENOENT:
#if 0
						throw Lookup_failed();
#else
						// NTFS-3g returns that in seemingly normal operation, yet we want to proceed rather than fail
						Genode::warning("ENOENT ignored, reading directory anyway");
						break;
#endif
					case ENOTDIR:
						return OPENDIR_ERR_LOOKUP_FAILED;
					case ENOSPC:
						Genode::error("op.opendir() error while expanding directory");
						return OPENDIR_ERR_LOOKUP_FAILED;
					case EROFS:
						return OPENDIR_ERR_PERMISSION_DENIED;
					default:
						Genode::error("op.opendir() returned unexpected error code: ", res);
						return OPENDIR_ERR_LOOKUP_FAILED;
				}
			}
			
			// instead of allocating the Dir_vfs_handle at the very end, maybe I should do this:
			/* "attempt allocation before modifying blocks" */

			try {
				*handle = new (alloc) Dir_vfs_handle(*this, *this, alloc);//, file);
//					Single_vfs_dir_handle(*this, *this, alloc,
//					                      _type, _rwx, _filename);
			}
			catch (Genode::Out_of_ram)  { return OPENDIR_ERR_OUT_OF_RAM; }
			catch (Genode::Out_of_caps) { return OPENDIR_ERR_OUT_OF_CAPS; }

			return OPENDIR_OK;
		}

		//+ openlink() ? (call Fuse::support_symlinks() first)

		void close(Vfs_handle *vfs_handle) override
		{
			///later: call sync() or sync_state() first ?

			TRACE( "close" );

			if (vfs_handle && (&vfs_handle->ds() == this)) {
				destroy(vfs_handle->alloc(), vfs_handle);
			}
		}

//		Watch_result watch(char const      *path, ..

		Genode::Dataspace_capability dataspace(char const *) override
		{
			Genode::warning(__func__, " not implemented in fuse plugin");
			return Genode::Dataspace_capability();
		}

		void release(char const *,
		             Genode::Dataspace_capability) override
		{
			Genode::warning(__func__, " (of dataspace) not implemented in fuse plugin");
		}

		file_size num_dirent(char const *path) override
		{
			TRACE( "num_dirent path=", path );

			char buf[4096];

			Genode::memset(buf, 0, sizeof (buf));

			struct fuse_dirhandle dh = {
				.filler = Fuse::fuse()->filler,
				.buf    = buf,
				.size   = sizeof (buf),
				.offset = 0,
			};

	//
			struct fuse_file_info  _file_info;///
			int res = Fuse::fuse()->op.opendir(path, &_file_info);
	//
			if (res == 0) {
				res = Fuse::fuse()->op.readdir(path/*.base()*/, &dh,
					Fuse::fuse()->filler, 0,
					&_file_info);
			}

			if (res != 0)
				return 0;

			return dh.offset / sizeof (struct dirent);
		}

		bool directory(char const *path) override
		{
			TRACE( "directory path=", path );

			struct stat s;
			if (Fuse::fuse()->op.getattr(path, &s) != 0 || ! S_ISDIR(s.st_mode))
				return false;

			return true;
		}

		char const *leaf_path(char const *path) override
		{
			TRACE( "leaf_path path=", path );

			struct stat s;

			return (Fuse::fuse()->op.getattr(path, &s) == 0)
				? path
				: nullptr;
		}

		Stat_result stat(char const *path, Stat &out) override
		{
			///later: call sync() first ?

			TRACE( "stat path=", path );

			out = Stat { };

			struct stat s;
			int res = Fuse::fuse()->op.getattr(path, &s);
			if (res != 0)
				return STAT_ERR_NO_ENTRY;

			out.device = (Genode::addr_t)this;
			out.inode = s.st_ino ? s.st_ino : 1;  // dodge "0" inode as that would break LibC
			out.size  = s.st_size;
			out.rwx   = Node_rwx::rw();//?

			if (S_ISDIR(s.st_mode))
				out.type  = Node_type::DIRECTORY;
			else if (S_ISREG(s.st_mode))
				out.type  = Node_type::CONTINUOUS_FILE;
			else if (S_ISLNK(s.st_mode))
				out.type  = Node_type::SYMLINK;
			else
				return STAT_ERR_NO_ENTRY;  // as in the former implementation's directory.h

			return STAT_OK;
		}

		Unlink_result unlink(char const *path) override
		{
			TRACE( "unlink path=", path );

			//if (!_writeable)
			//+	throw Permission_denied();

			int res = Fuse::fuse()->op.unlink(path);
			if (res != 0) {
				Genode::error("fuse()->op.unlink() returned unexpected error code: ", res);
				return UNLINK_ERR_NO_ENTRY;
			}

			return UNLINK_OK;
		}

		Rename_result rename(char const *from, char const *to) override
		{
			TRACE( "rename from=", from, " to=", to );

			int res = Fuse::fuse()->op.rename(from,//absolute_to_path.base(),
	                                  	  	  to);//absolute_from_path.base());
			if (res != 0) {
				Genode::error("fuse()->op.rename() returned unexpected error code: ", res);
				return RENAME_ERR_NO_ENTRY;
			}

			///ToDo: should we update any open File_vfs_handle's path ?

			return RENAME_OK;
		}

		/************************
		 ** File I/O interface **
		 ************************/

		Write_result write(Vfs_handle *vfs_handle,
		                   char const *buf, file_size buf_size,
		                   file_size &out_count) override
		{
			TRACE( "file.write bufsize=", buf_size );

			auto *handle = dynamic_cast<File_vfs_handle*>(vfs_handle);
			if (!handle)
				return WRITE_ERR_INVALID;//
			//+test OPEN_MODE_RDONLY ?

			return
				handle->write (buf, buf_size, out_count);
		}

		Read_result complete_read(Vfs_handle *vfs_handle, char *buf,
		                          file_size buf_size,
		                          file_size &out_count) override
		{
			TRACE( "file.complete_read bufsize=", buf_size );
			
			auto *handle = dynamic_cast<File_vfs_handle*>(vfs_handle);
			if (!handle)
				return READ_ERR_INVALID;//

			return
				handle->read (buf, buf_size, out_count);
		}

		Ftruncate_result ftruncate(Vfs_handle *vfs_handle, file_size len) override
		{
			TRACE( "file.ftruncate len=", len );

			auto *handle = dynamic_cast<File_vfs_handle*>(vfs_handle);
			if (!handle)
				return FTRUNCATE_ERR_NO_PERM;//

			return
				handle->ftruncate (len);
		}

		bool read_ready(Vfs_handle *) override { return true; }
};


extern "C" Vfs::File_system_factory *vfs_file_system_factory(void)
{
	struct Factory : Vfs::File_system_factory
	{
		Vfs::File_system *create(Vfs::Env &vfs_env, Genode::Xml_node config) override
		{
			return new (vfs_env.alloc()) Fuse_file_system(vfs_env, config);
		}
	};

	static Factory f;
	return &f;
}

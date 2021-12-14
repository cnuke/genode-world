/*
 * Copyright (C) 2021 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__FUSE__blocks_as_file_H_
#define _INCLUDE__FUSE__blocks_as_file_H_

/* Genode includes */
#include <base/allocator.h>
#include <os/vfs.h>  // Readonly_file

//
using namespace Genode;



class Readwrite_file : public File
{
	// The FUSE device, e.g. "/dev/blkdev", is read from (and written to) like
	// a VFS file, instead of a Block device. This class translates posix-style
	// read()/write() calls coming from FUSE, into VFS file calls (which, behind
	// the scenes, will come out as Block device calls).
	
	private:

		/*
		 * Noncopyable
		 */
		Readwrite_file(Readwrite_file const &);
		Readwrite_file &operator = (Readwrite_file const &);

		Vfs::Vfs_handle *_handle = nullptr;

		Genode::Entrypoint &_ep;

		void _open(Vfs::File_system &fs, Allocator &alloc, Path const path)
		{
			Vfs::Directory_service::Open_result res =
				fs.open(path.string(), Vfs::Directory_service::OPEN_MODE_RDWR,//RDONLY,
				        &_handle, alloc);

			if (res != Vfs::Directory_service::OPEN_OK) {
				error("failed to open file '", path, "'");
				throw Open_failed();
			}
		}

		/**
		 * Strip off constness of 'Directory const &'
		 *
		 * Since the 'Readonly_file' API provides an abstraction over the
		 * low-level VFS operations, the intuitive meaning of 'const' is
		 * different between the 'Readonly_file' API and the VFS.
		 *
		 * At the VFS level, opening a file changes the internal state of the
		 * VFS. Hence the operation is non-const. However, the user of the
		 * 'Readonly_file' API expects the constness of a directory to
		 * correspond to whether the directory can be modified or not. In the
		 * case of instantiating a 'Readonly_file', one would expect that a
		 * 'Directory const &' would suffice. The fact that - under the hood -
		 * the 'Readonly_file' has to perform the nonconst 'open' operation at
		 * the VFS is of not of interest.
		 */
#if 1//
		static Directory &_mutable(Directory const &dir)
		{
			return const_cast<Directory &>(dir);
		}
#endif

	public:

		/**
		 * Constructor
		 *
		 * \throw File::Open_failed
		 */
		Readwrite_file(Path const &full_path, Vfs::Env &vfs_env)
		: _ep(vfs_env.env().ep())
		{
#if 0
	// dir's members are private, can't access dir's _fs, _alloc, _path ...
			_open(_mutable(dir)._fs, _mutable(dir)._alloc,
			      Directory::join(dir._path, rel_path));
#else
	// so do this instead:
			_open(
				vfs_env.root_dir(),
				vfs_env.alloc(),
				full_path);
#endif
		}

		~Readwrite_file()
		{
///+++
//..			while (_handle.fs().queue_sync(&_handle) == false)
//..				_ep.wait_and_dispatch_one_io_signal();
//..
			_handle->ds().close(_handle);
		}

		struct At { Vfs::file_size value; };

		/**
		 * Write 'size' bytes to file from local memory buffer 'src'
		 */
		size_t write(At at, char const *src, size_t const size)
		{
			_handle->seek(at.value);

			bool write_error = false;
			size_t remaining_bytes = size;

			while (remaining_bytes > 0 && !write_error) {

				bool stalled = false;

				try {
					Vfs::file_size out_count = 0;

					using Write_result = Vfs::File_io_service::Write_result;

					switch (_handle->fs().write(_handle, src, remaining_bytes,
					                           out_count)) {

					case Write_result::WRITE_ERR_AGAIN:
					case Write_result::WRITE_ERR_WOULD_BLOCK:
						stalled = true;
						break;

					case Write_result::WRITE_ERR_INVALID:
					case Write_result::WRITE_ERR_IO:
					case Write_result::WRITE_ERR_INTERRUPT:
						write_error = true;
						break;

					case Write_result::WRITE_OK:
						out_count = min(remaining_bytes, out_count);
						remaining_bytes -= out_count;
						src             += out_count;
						_handle->advance_seek(out_count);
						break;
					};
				}
				catch (Vfs::File_io_service::Insufficient_buffer) {
					stalled = true; }

				if (stalled)
					_ep.wait_and_dispatch_one_io_signal();
			}

//			return write_error ? Append_result::WRITE_ERROR
//			                   : Append_result::OK;
			return size - remaining_bytes;//out_count;
		}

		/**
		 * Read number of 'bytes' from file into local memory buffer 'dst'
		 *
		 * \throw Truncated_during_read
		 */
		size_t read(At at, char *dst, size_t bytes) const
		{
			Vfs::file_size out_count = 0;

			_handle->seek(at.value);

			while (!_handle->fs().queue_read(_handle, bytes))
				_ep.wait_and_dispatch_one_io_signal();

			Vfs::File_io_service::Read_result result;

			for (;;) {
				result = _handle->fs().complete_read(_handle, dst, bytes,
				                                     out_count);

				if (result != Vfs::File_io_service::READ_QUEUED)
					break;

				_ep.wait_and_dispatch_one_io_signal();
			};

			/*
			 * XXX handle READ_ERR_AGAIN, READ_ERR_WOULD_BLOCK, READ_QUEUED
			 */

			if (result != Vfs::File_io_service::READ_OK)
				throw Truncated_during_read();

			return out_count;
		}

#if 0
	unexpected semantics, seems dangerous...?
		/**
		 * Read number of 'bytes' from the start of the file into local memory
		 * buffer 'dst'
		 *
		 * \throw Truncated_during_read
		 */
		size_t read(char *dst, size_t bytes) const
		{
			return read(At{0}, dst, bytes);
		}
#endif
};


struct Blocks_as_file
{
	Vfs::Env &_vfs_env;

	//Constructible<Genode::Readonly_file>  file;
	Constructible<Readwrite_file>  file;

	Blocks_as_file(Vfs::Env &vfsenv)
	: _vfs_env(vfsenv),
	  file()
	{
	}

	void open(const char * path);
	int pread(int fd, char * buf, size_t count, size_t offset);
	int pwrite(int fd, const char * buf, size_t count, size_t offset);
};


#endif // _INCLUDE__FUSE__blocks_as_file_H_


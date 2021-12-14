/*
 * \brief  Libc functions for FUSE (& lwext4)
 * \author Josef Soentgen
 * \date   2017-08-01
 */

// We can't use the actual LibC in a VFS plug-in, so create fake
// read/write/malloc etc calls that forward to pure Genode APIs.

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


/* Genode includes */
#include <base/allocator.h>
#include <base/allocator_avl.h>
#include <base/log.h>
#include <block_session/connection.h>
#include <log_session/log_session.h>

//libc
#include <stdio.h>

/* FUSe includes */
#include <fuse.h>
#include <fuse_private.h>

/* local includes */
#if 0
#include "block_dev.h"
extern Blockdev * ptr_to_blockdev;

#warning error use Block_FS instead of Block !
/*
	otherwise we get non-aligned write access, which is a no-no:

[init -> test-libc_vfs] -- Starting fuse vfs plug-in --
[init -> test-libc_vfs] libc_fuse_ntfs-3g: try to mount /dev/blkdev...
..
[init -> test-libc_vfs] calling mkdir(trailing_slash_dir_name, 0777) dir_name=testdir/
[init -> test-libc_vfs] ---->leaf_path path=/testdir
[init -> test-libc_vfs] ---->opendir path=/testdir create=1
.. [init -> test-libc_vfs] .fuse-PWRITE fd=0 count=4088 offset=8200
[init -> test-libc_vfs] Error: pwrite: offset is not a multiple of sector size ! not implemented !
[init -> test-libc_vfs] Error: pwrite: count is not a multiple of sector size ! not implemented !
[init -> test-libc_vfs] Error: fuselibc_pwrite failed at sector 16 -- p.size: 3584
qemu-system-x86_64: ahci: PRDT length for NCQ command (0x1) is smaller than the requested size (0x2000000)
*/
#else
#include "blocks_as_file.h"



void Blocks_as_file::open( const char * path )
{
	Genode::log( " --block-as-file.open(", path, ") ---" );
	
	Genode::Directory dir( _vfs_env );
	file.construct( /*dir, */path, _vfs_env );
	///ToDo: Dunno how to pass dir's private _path, so assuming it's equal to "/", seems to work?
}

int Blocks_as_file::pread( int fd, char * buf, size_t count, size_t offset )
{
	//Genode::log( "  Blocks_as_file::pread fd=", fd, " with file.constructed=", file.constructed());
	
	///? if file.constructed()
	///? while (total_read < _buffer.size) ...
	Vfs::file_size read_bytes =
		file->read(
			Readwrite_file::At{offset},
			buf,
			count
			);
	if (read_bytes != count)
		Genode::error( "failed to read some/all bytes (", read_bytes, " != ", count, ")" );

	return read_bytes;
}

int Blocks_as_file::pwrite( int fd, const char * buf, size_t count, size_t offset )
{
	//Genode::log( "  Blocks_as_file::pwrite fd=", fd, " with file.constructed=", file.constructed());
	
	///? if file.constructed()
	///? while (total_ < _buffer.size) ...
	Vfs::file_size written_bytes =
		file->write(
			Readwrite_file::At{offset},
			buf,
			count
			);
	if (written_bytes != count)
		Genode::error( "failed to write some/all bytes (", written_bytes, " != ", count, ")" );

	return written_bytes;
}


extern Blocks_as_file * ptr_to_blockdev;

#endif




/*
 * Genode enviroment
 */
static Genode::Env       *_global_env;
static Genode::Allocator *_global_alloc;

void /*Lwext4::*/malloc_init(Genode::Env &env, Genode::Allocator &alloc)
{
	_global_env   = &env;
	_global_alloc = &alloc;
}



extern "C" {
	// stat(), etc will get remapped to fuselibc_stat(), etc
	// thanks to -Dopen=fuselibc_stat macro & friends


// ** errno.h **

int *	__error(void)
{
	//Genode::error("accessing errno");
	
	static int dummy;
	return &dummy;
}



/*************
 ** stdio.h **
 *************/

///most of the below are NOT yet aliased with "-D..." but they seem to correctly override their LibC counterpart as needed anyhow

int fflush(FILE *)
{
	return 0;
}


int vfprintf( FILE * /*stream*/, const char * fmt, __va_list list)
{
	static/**/ char buffer[Genode::Log_session::MAX_STRING_LEN];

	Genode::String_console sc(buffer, sizeof (buffer));
	sc.vprintf(fmt, list);

	/* remove newline as Genode::log() will implicitly add one */
	size_t n = sc.len();
	if (n > 0 && buffer[n-1] == '\n') { n--; }

	Genode::log(Genode::Cstring(buffer, n));
	
	return 0;
}


//int printf(char const *fmt, ...)
int fprintf(FILE *, char const *fmt, ...)
{
	static/**/ char buffer[Genode::Log_session::MAX_STRING_LEN];

	va_list list;
	va_start(list, fmt);
	Genode::String_console sc(buffer, sizeof (buffer));
	sc.vprintf(fmt, list);
	va_end(list);

	/* remove newline as Genode::log() will implicitly add one */
	size_t n = sc.len();
	if (n > 0 && buffer[n-1] == '\n') { n--; }

	Genode::log(Genode::Cstring(buffer, n));

	return 0;
}

int puts(const char *s)
{
//	printf("%s\n", s);
	fprintf(stdout, "%s\n", s);
	return 0;
}


/**************
 ** stdlib.h **
 **************/

void *malloc(size_t sz)
{
	return _global_alloc->alloc(sz);
}


void *calloc(size_t n, size_t sz)
{
	/* XXX overflow check */
	size_t size = n * sz;
	void *p = malloc(size);
	if (p) { Genode::memset(p, 0, size); }
	return p;
}

void free(void *p)
{
	if (p == NULL) { return; }

	_global_alloc->free(p, 0);
}

void *realloc(void *p, size_t n)
{
#if 1
	/* naive/inefficient implementation: copy all to new buffer */

	void *newer = malloc(n);
	if (!newer)
		// don't free() the old buf, the app can continue to use it ?
		return NULL;
///FIXME: tx'ing as much as "new_size" bytes from the OLD buffer (which might be smaller -- or worse, bigger) is going to segfault one day or another !
//maybe look at repos/base/src/lib/cxx/malloc_free.cc ?
//   or look at repos/libports/src/lib/libc ?
	size_t old_size = n; // :-(
	if (p) Genode::memcpy (newer, p, old_size);
	free (p);
	return newer;
#else
// no such function/does not exist...
	return _global_alloc->realloc(p, n);
#endif
}


char * strdup( const char * str )
{
	//assert(str != nullptr);
	Genode::size_t const size = Genode::strlen(str) + sizeof('\0');
	char * copy = (char*)malloc(size);
	Genode::copy_cstring(copy, str, size);
	
	return copy;
}


int	stat(const char * path, struct stat * fields)
{
	Genode::log( ".fuse-Stat: ", path );
	
	*fields = {};//fields->st_mode = 0;
	
	///?  st_size = ptr_to_blockdev->info.block_count * _info.block_size ...
	
	//no point, ntfs-3g STILL asks us non-whole-sectors, even if we specify S_ISBLK... So let's add support for that
	//fields->st_mode = 0060000;
	
	return 0;// -1;
}



int	open(const char * path, int, ...)
{
	Genode::log( ".fuse-Open: ", path );

	ptr_to_blockdev->open(path);

	return 0;///come up with a file descriptor scheme, other than "let's just always return fd 0" ! (though in the case of ntfs-3g, this horror does work...)
}


//int pread( int fd, char * buf, int64_t count, int64_t offset )
int pread(int fd, char * buf, size_t count, size_t offset)
{
	//Genode::log( ".fuse-PRead fd=", fd, " count=", count, " offset=", offset );
	
	return ptr_to_blockdev->pread(fd, buf, count, offset);
}

int pwrite(int fd, const char * buf, size_t bytecount, size_t byteoffset)
{
	//Genode::log( ".fuse-PWRITE fd=", fd, " count=", bytecount, " offset=", byteoffset );
	
	// fuse/ntfs writes to 'random' (non-aligned) beginning/end offsets, so we
	// have to convert those into 512-bytes-aligned offsets, as needed by
	// BlockDev... AFTER retrieving the previous block contents in the "gray zone"
	// between aligned and non aligned offsets, so that we don't write back zeros (!):
	
/*
	... read old contents..
	.... substitute new contents, at the one place <byteoffset> - <byteoffset of sector>
	... write back modified block...
	
	Genode::size_t const sector_size = ptr_to_blockdev->_info.block_size;
	first_sector = byteoffset / sector_size;
	
	return ptr_to_blockdev->pwrite(
		buf,
		bytecount,
		first_sector * sector_size//byteoffset
		);
Nah, let's use Vfs-Blocks instead, which means no change at our end:
*/
	return ptr_to_blockdev->pwrite(fd, buf, bytecount, byteoffset);
}



///ToDo: so I no longer need a patch in Genode's libc's fcntl(), right ?
int	fcntl(int fd, int, ...)
{
	Genode::log( ".fuse-Fcntl fd=", fd );
	return 0;//-1;
}


off_t lseek(int fd, off_t off, int mode)
{
	Genode::log( ".fuse-Lseek fd=", fd, " off=", off, " mode=", mode );
	
	off_t new_pos = 0;
	//switch( mode )
	// case SEEK_SET.. SEEK_CUR.. SEEK_END..
	//// new_pos = old_pos + ..
	
	///if new_pos >= partition_bytesize
	///	return -1;
	
	Genode::warning( "*** lseek not implemented, remaining as-is, FUSE probably won't like this" );
	
	return new_pos;
}

int fsync()
{
	Genode::log( ".fuse-FSync" );
	return -1;
}

int close()
{
	Genode::log( ".fuse-Close" );
	return -1;
}

}  // ~extern "C"


SRC_CC = fuse.cc

libc_PORT_DIR := $(call select_from_ports,libc)

INC_DIR += $(REP_DIR)/include/fuse \
		   $(libc_PORT_DIR)/include/libc \
		   $(libc_PORT_DIR)/include/spec/x86_64/libc
#///ToDo: instead of hardcoding "x86_64", find a way to "include repos/libports/lib/import/import-libc.mk"

# We are linked against VFS plug-ins, which cannot be compiled with "LIBS = libc".
# So alias the needed LibC symbols to our simplified implementation instead:
# CC_OPT += -Dopen=fuselibc_open -Dclose=fuselibc_close \
# 	-Dpread=fuselibc_pread -Dpwrite=fuselibc_pwrite \
# 	-Dstat=fuselibc_stat -Dseek=fuselibc_seek -Dsync=fuselibc_sync \
# 	-Dfstat=fuselibc_fstat -Dlseek=fuselibc_lseek -Dfsync=fuselibc_fsync \
# 	-Dioctl=fuselibc_ioctl -Dfcntl=fuselibc_fcntl \
# 	-Dmalloc=fuselibc_malloc -Dcalloc=fuselibc_calloc \
# 	-Dfprintf=fuselibc_fprintf
	#-Dread=fuselibc_read -Dwrite=fuselibc_write : messes with Readonly_File.read()
	#-Dfree=fuselibc_free  -Drealloc=fuselibc_realloc : messes with avlallocator.free()

CC_OPT += -fpermissive

vpath %.cc $(REP_DIR)/src/lib/fuse

CC_CXX_WARN_STRICT =

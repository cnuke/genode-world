NTFS_3G_PORT_DIR := $(call select_from_ports,ntfs-3g)
NTFS_3G_DIR      := $(NTFS_3G_PORT_DIR)/src/lib/ntfs-3g

libc_PORT_DIR := $(call select_from_ports,libc)


FILTER_OUT = win32_io.c
SRC_C = $(filter-out $(FILTER_OUT),$(notdir $(wildcard $(NTFS_3G_DIR)/libntfs-3g/*.c)))

INC_DIR += $(REP_DIR)/include/ntfs-3g \
           $(REP_DIR)/src/lib/ntfs-3g \
           $(NTFS_3G_PORT_DIR)/include/ntfs-3g \
		   $(libc_PORT_DIR)/include/libc \
		   $(libc_PORT_DIR)/include/spec/x86_64/libc
#///ToDo: instead of hardcoding "x86_64", find a way to "include repos/libports/lib/import/import-libc.mk"

CC_OPT += -DHAVE_CONFIG_H -DRECORD_LOCKING_NOT_IMPLEMENTED -DDEBUG

#===============================
CC_OPT += -Dopen=fuselibc_open -Dclose=fuselibc_close \
	-Dpread=fuselibc_pread -Dpwrite=fuselibc_pwrite \
	-Dstat=fuselibc_stat -Dseek=fuselibc_seek -Dsync=fuselibc_sync \
	-Dfstat=fuselibc_fstat -Dlseek=fuselibc_lseek -Dfsync=fuselibc_fsync \
	-Dioctl=fuselibc_ioctl -Dfcntl=fuselibc_fcntl \
	-Dmalloc=fuselibc_malloc -Dcalloc=fuselibc_calloc \
	-Dfprintf=fuselibc_fprintf
	#-Dread=fuselibc_read -Dwrite=fuselibc_write : messes with Readonly_File.read()
	#-Dfree=fuselibc_free  -Drealloc=fuselibc_realloc : messes with avlallocator.free()

vpath %.c $(NTFS_3G_DIR)/libntfs-3g

CC_CXX_WARN_STRICT =

SHARED_LIB := yes
SRC_CC     := vfs.cc block.cc
LIBS       := base lwext4

VFS_DIR = $(REP_DIR)/src/lib/vfs/lwext4
INC_DIR += $(VFS_DIR)

LD_OPT += --version-script=$(VFS_DIR)/symbol.map

CC_OPT += -DCONFIG_USE_DEFAULT_CFG=1
CC_OPT += -DCONFIG_HAVE_OWN_ERRNO=1
CC_OPT += -DCONFIG_HAVE_OWN_ASSERT=1
CC_OPT += -DCONFIG_BLOCK_DEV_CACHE_SIZE=256

vpath %.cc $(VFS_DIR)

CC_CXX_WARN_STRICT =

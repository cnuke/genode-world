TARGET   = lwext4_fs
SRC_CC   = main.cc file_system.cc block.cc
LIBS     = base lwext4

INC_DIR += $(PRG_DIR)

CC_OPT += -DCONFIG_USE_DEFAULT_CFG=1
CC_OPT += -DCONFIG_HAVE_OWN_ERRNO=1
CC_OPT += -DCONFIG_HAVE_OWN_ASSERT=1
CC_OPT += -DCONFIG_BLOCK_DEV_CACHE_SIZE=256

CC_CXX_WARN_STRICT =

SRC_CC = vfs.cc

DDE_LINUX_DIR := $(subst /src/include/lx_kit,,$(call select_from_repositories,src/include/lx_kit))

INC_DIR += $(DDE_LINUX_DIR)/src/include

LIBS := wifi_ath9k

vpath %.cc $(REP_DIR)/src/lib/vfs/wifi_ath9k

SHARED_LIB := yes

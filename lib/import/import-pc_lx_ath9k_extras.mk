LX_EXTRA_SRC_DIR := $(call select_from_ports,linux)/src/linux
ifeq ($(wildcard $(LX_EXTRA_SRC_DIR)),)
LX_EXTRA_SRC_DIR := $(call select_from_repositories,src/linux_extras)
ifeq ($(wildcard $(LX_EXTRA_SRC_DIR)),)
$(error CAN NOT FIND linux_extras DIRECTORY)
else
vpath %.c $(LX_EXTRA_SRC_DIR)
vpath %.S $(LX_EXTRA_SRC_DIR)
INC_DIR += $(LX_EXTRA_SRC_DIR)/arch/$(LX_ARCH)/include
INC_DIR += $(LX_EXTRA_SRC_DIR)/include
INC_DIR += $(LX_EXTRA_SRC_DIR)/arch/$(LX_ARCH)/include/uapi
INC_DIR += $(LX_EXTRA_SRC_DIR)/include/uapi
endif
endif

SHARED_LIB =1

LIBMBIM_PORT_DIR = $(call select_from_ports,libmbim)

LIBMBIM_SRC_DIR = $(LIBMBIM_PORT_DIR)/src/lib/libmbim/src

COMMON_SRC_C := $(notdir $(wildcard $(LIBMBIM_SRC_DIR)/common/*.c))

SRC_C += $(COMMON_SRC_C)

INC_DIR += $(REP_DIR)/src/lib/libmbim

vpath %.c $(LIBMBIM_SRC_DIR)/common

LIBS += libc libiconv glib

CC_CXX_WARN_STRICT =

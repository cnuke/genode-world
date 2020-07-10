SHARED_LIB =1

LIBMBIM_PORT_DIR = $(call select_from_ports,libmbim)
GLIB_PORT_DIR = $(call select_from_ports,glib)

LIBMBIM_SRC_DIR = $(LIBMBIM_PORT_DIR)/src/lib/libmbim/src
GLIB_SRC_DIR = $(GLIB_PORT_DIR)/src/lib/glib/glib
GLIB_SRC_DIR = $(GLIB_PORT_DIR)/src/lib/glib/glib

COMMON_SRC_C := $(notdir $(wildcard $(LIBMBIM_SRC_DIR)/common/*.c))

LIBMBIM_GLIB_SRC_C := $(notdir $(wildcard $(LIBMBIM_SRC_DIR)/libmbim-glib/*.c))

CC_DEF += -DLIBMBIM_GLIB_COMPILATION
CC_DEF += -DG_GNUC_INTERNAL=""

#
# These are all dummy values in the defines. It is assumed that these defines
# are used only for the socket back end. Using the socket back end is not
# supported for now.
#
CC_DEF += -DGLIB_SYSDEF_AF_INET6=1
CC_DEF += -DGLIB_SYSDEF_AF_UNIX=1
CC_DEF += -DGLIB_SYSDEF_AF_INET=1
CC_DEF += -DGLIB_SYSDEF_MSG_OOB=1
CC_DEF += -DGLIB_SYSDEF_MSG_PEEK=1
CC_DEF += -DGLIB_SYSDEF_MSG_PEEK=1
CC_DEF += -DGLIB_SYSDEF_MSG_DONTROUTE=1
CC_DEF += -DGLIB_SYSDEF_MSG_DONTROUTE=1
CC_DEF += -DGLIB_SYSDEF_AF_UNIX=1
CC_DEF += -DGLIB_SYSDEF_AF_INET=1
CC_DEF += -DLIBEXEC_PATH=

SRC_C += $(COMMON_SRC_C)
SRC_C += $(LIBMBIM_GLIB_SRC_C)

LIBC_PORT_DIR := $(call select_from_ports,libc)
ifeq ($(filter-out $(SPECS),x86_64),)
	INC_DIR += $(LIBC_PORT_DIR)/include/spec/x86_64/libc/machine/
endif
ifeq ($(filter-out $(SPECS),x86_32),)
	INC_DIR += $(LIBC_PORT_DIR)/include/spec/x86_32/libc/machine/
endif
ifeq ($(filter-out $(SPECS),arm_64),)
	INC_DIR += $(LIBC_PORT_DIR)/include/spec/arm_64/libc/machine/
endif
ifeq ($(filter-out $(SPECS),arm),)
	INC_DIR += $(LIBC_PORT_DIR)/include/spec/arm/libc/machine/
endif

INC_DIR += $(REP_DIR)/src/lib/libmbim
INC_DIR += $(REP_DIR)/src/lib/libmbim/generated
INC_DIR += $(GLIB_SRC_DIR)
INC_DIR += $(GLIB_PORT_DIR)/src/lib/glib
INC_DIR += $(GLIB_PORT_DIR)/src/lib/glib/gmodule
INC_DIR += $(LIBMBIM_SRC_DIR)/libmbim-glib
INC_DIR += $(LIBMBIM_SRC_DIR)/common

vpath %.c $(LIBMBIM_SRC_DIR)/common
vpath %.c $(LIBMBIM_SRC_DIR)/libmbim-glib

LIBS += libc libiconv glib

CC_CXX_WARN_STRICT =

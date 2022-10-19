TARGET := display_fetchurl_progress
LIBS   := base lvgl libc

LIBLVGL_PORT_DIR := $(call select_from_ports,lvgl)
LIBLVGL_SRC_DIR  := $(LIBLVGL_PORT_DIR)/src/lib/lvgl

INC_DIR += $(LIBLVGL_SRC_DIR)

SRC_CC := main.cc

LIBLVGL_SUPPORT_DIR := $(REP_DIR)/src/lib/lvgl_support
SRC_CC  += lvgl_support.cc
INC_DIR += $(LIBLVGL_SUPPORT_DIR)

INC_DIR += $(PRG_DIR)
SRC_C += ui.c \
         ui_helpers.c

vpath %.c  $(LIBLVGL_SRC_DIR)
vpath %.c  $(PRG_DIR)
vpath %.cc $(LIBLVGL_SUPPORT_DIR)

CC_CXX_WARN_STRICT :=

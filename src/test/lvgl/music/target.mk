TARGET := test-lvgl-music
LIBS   := base lvgl libc

LIBLVGL_PORT_DIR := $(call select_from_ports,lvgl)
LIBLVGL_SRC_DIR  := $(LIBLVGL_PORT_DIR)/src/lib/lvgl

SRC_CC += main.cc

LIBLVGL_SUPPORT_DIR := $(REP_DIR)/src/lib/lvgl_support
SRC_CC  += lvgl_support.cc
INC_DIR += $(LIBLVGL_SUPPORT_DIR)

INC_DIR += $(LIBLVGL_SRC_DIR)
INC_DIR += $(LIBLVGL_SRC_DIR)/demos

SRC_C += $(addprefix demos/music/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/demos/music/*.c)))
SRC_C += $(addprefix demos/music/assets/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/demos/music/assets/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/demos/music
INC_DIR += $(LIBLVGL_SRC_DIR)/demos/music/assets

INC_DIR += $(PRG_DIR)

$(info $(LIBLVGL_SRC_DIR))

vpath %.c $(LIBLVGL_SRC_DIR)
vpath %.c $(PRG_DIR)
vpath %.cc $(LIBLVGL_SUPPORT_DIR)

CC_CXX_WARN_STRICT :=

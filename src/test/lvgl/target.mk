TARGET := test-lvgl
LIBS   := base lvgl libc

LIBLVGL_PORT_DIR := $(call select_from_ports,lvgl)
LIBLVGL_SRC_DIR  := $(LIBLVGL_PORT_DIR)/src/lib/lvgl

SRC_CC := main.cc
SRC_C  := mouse_cursor_icon.c

SRC_C += $(addprefix demos/music/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/demos/music/*.c)))
SRC_C += $(addprefix demos/music/assets/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/demos/music/assets/*.c)))

INC_DIR += $(LIBLVGL_SRC_DIR)
INC_DIR += $(LIBLVGL_SRC_DIR)/demos
INC_DIR += $(LIBLVGL_SRC_DIR)/demos/music
INC_DIR += $(LIBLVGL_SRC_DIR)/demos/music/assets

INC_DIR += $(PRG_DIR)

$(info $(LIBLVGL_SRC_DIR))

vpath %.c $(LIBLVGL_SRC_DIR)

CC_CXX_WARN_STRICT :=

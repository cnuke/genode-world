TARGET := test-lvgl-widgets
LIBS   := base lvgl libc

LIBLVGL_PORT_DIR := $(call select_from_ports,lvgl)
LIBLVGL_SRC_DIR  := $(LIBLVGL_PORT_DIR)/src/lib/lvgl

SRC_CC += main.cc

LIBLVGL_SUPPORT_DIR := $(REP_DIR)/src/lib/lvgl_support
SRC_CC  += lvgl_support.cc
INC_DIR += $(LIBLVGL_SUPPORT_DIR)

INC_DIR += $(LIBLVGL_SRC_DIR)
INC_DIR += $(LIBLVGL_SRC_DIR)/demos

SRC_C += $(addprefix demos/widgets/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/demos/widgets/*.c)))
SRC_C += $(addprefix demos/widgets/assets/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/demos/widgets/assets/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/demos/widgets
INC_DIR += $(LIBLVGL_SRC_DIR)/demos/widgets/assets

INC_DIR += $(PRG_DIR)

$(info $(LIBLVGL_SRC_DIR))

vpath %.c $(LIBLVGL_SRC_DIR)
vpath %.c $(PRG_DIR)
vpath %.cc $(LIBLVGL_SUPPORT_DIR)

CC_CXX_WARN_STRICT :=

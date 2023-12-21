include $(REP_DIR)/lib/import/import-lvgl.mk

LIBLVGL_SRC_DIR := $(LIBLVGL_PORT_DIR)/src/lib/lvgl

CC_CXX_OPT += -Wno-deprecated-enum-enum-conversion

SRC_CC := glue.cc

SRC_C_core := \
	$(addprefix core/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/core/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/core

SRC_C_draw := \
	$(addprefix draw/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/draw/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/draw

SRC_C_draw_sw := \
	$(addprefix draw/sw/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/draw/sw/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/draw/sw

SRC_C_extra := \
	$(addprefix extra/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/extra

SRC_C_font := \
	$(addprefix font/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/font/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/font

SRC_C_hal := \
	$(addprefix hal/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/hal/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/hal

SRC_C_misc := \
	$(addprefix misc/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/misc/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/misc

SRC_C_widgets := \
	$(addprefix widgets/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/widgets/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/widgets

SRC_C := $(SRC_C_core) $(SRC_C_draw) $(SRC_C_draw_sw) $(SRC_C_extra) $(SRC_C_font) \
         $(SRC_C_hal) $(SRC_C_misc) $(SRC_C_widgets)

SRC_C_extra_layouts := \
	$(addprefix extra/layouts/flex/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/layouts/flex/*.c))) \
	$(addprefix extra/layouts/grid/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/layouts/grid/*.c)))

INC_DIR += \
	$(LIBLVGL_SRC_DIR)/src/extra/layouts \
	$(LIBLVGL_SRC_DIR)/src/extra/layouts/flex \
	$(LIBLVGL_SRC_DIR)/src/extra/layouts/grid

SRC_C_extra_themes := \
	$(addprefix extra/themes/default/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/themes/default/*.c))) \
	$(addprefix extra/themes/basic/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/themes/basic/*.c)))
INC_DIR += $(LIBLVGL_SRC_DIR)/src/extra/themes/default \
           $(LIBLVGL_SRC_DIR)/src/extra/themes/basic

SRC_C_extra_widgets := \
	$(addprefix extra/widgets/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/*.c))) \
	$(addprefix extra/widgets/animimg/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/animimg/*.c))) \
	$(addprefix extra/widgets/calendar/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/calendar/*.c))) \
	$(addprefix extra/widgets/chart/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/chart/*.c))) \
	$(addprefix extra/widgets/colorwheel/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/colorwheel/*.c))) \
	$(addprefix extra/widgets/imgbtn/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/imgbtn/*.c))) \
	$(addprefix extra/widgets/keyboard/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/keyboard/*.c))) \
	$(addprefix extra/widgets/led/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/led/*.c))) \
	$(addprefix extra/widgets/list/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/list/*.c))) \
	$(addprefix extra/widgets/menu/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/menu/*.c))) \
	$(addprefix extra/widgets/meter/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/meter/*.c))) \
	$(addprefix extra/widgets/msgbox/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/msgbox/*.c))) \
	$(addprefix extra/widgets/span/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/span/*.c))) \
	$(addprefix extra/widgets/spinbox/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/spinbox/*.c))) \
	$(addprefix extra/widgets/spinner/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/spinner/*.c))) \
	$(addprefix extra/widgets/tabview/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/tabview/*.c))) \
	$(addprefix extra/widgets/tileview/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/tileview/*.c))) \
	$(addprefix extra/widgets/win/,$(notdir $(wildcard $(LIBLVGL_SRC_DIR)/src/extra/widgets/win/*.c)))

INC_DIR += \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/animimg \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/calendar \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/chart \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/colorwheel \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/imgbtn \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/keyboard \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/led \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/list \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/menu \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/meter \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/msgbox \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/span \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/spinbox \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/spinner \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/tabview \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/tileview \
	$(LIBLVGL_SRC_DIR)/src/extra/widgets/win

SRC_C += $(SRC_C_extra_layouts) $(SRC_C_extra_themes) $(SRC_C_extra_widgets)

INC_DIR += $(REP_DIR)/src/lib/lvgl

LIBS += libc

vpath %.c $(LIBLVGL_SRC_DIR)/src
vpath %.cc $(REP_DIR)/src/lib/lvgl

#SHARED_LIB := yes

CC_CXX_WARN_STRICT :=

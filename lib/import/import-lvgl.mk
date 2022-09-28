LIBLVGL_PORT_DIR := $(call select_from_ports,lvgl)
INC_DIR += $(LIBLVGL_PORT_DIR)/include/lvgl
INC_DIR += $(LIBLVGL_PORT_DIR)/src/lib/lvgl
INC_DIR += $(REP_DIR)/src/lib/lvgl
INC_DIR += $(LIBLVGL_PORT_DIR)/src/lib/lvgl/src
INC_DIR += $(REP_DIR)/src/lib/lvgl/src

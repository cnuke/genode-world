MIRROR_FROM_REP_DIR := \
	lib/import/import-lvgl.mk \
	lib/mk/lvgl.mk \
	$(shell cd $(REP_DIR); find src/lib/lvgl -type f) \
	$(shell cd $(REP_DIR); find src/lib/lvgl_support -type f) \
	$(shell cd $(REP_DIR); find src/test/lvgl/widgets -type f)

content: src/lib/lvgl $(MIRROR_FROM_REP_DIR) LICENSE \
         include/lvgl

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/lvgl)

src/lib/lvgl:
	mkdir -p $@
	cp -r $(PORT_DIR)/src/lib/lvgl/* $@

include/lvgl:
	mkdir -p $@
	cp -r $(PORT_DIR)/include/lvgl/* $@

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/lvgl/LICENCE.txt $@

LIB_MK_FILES := $(notdir $(wildcard $(REP_DIR)/lib/mk/yquake2*))
MIRROR_FROM_PORT_DIR := src/app/yquake2

MIRROR_FROM_REP_DIR := $(addprefix lib/mk/,$(LIB_MK_FILES)) \
                       src/lib/yquake2

content: $(MIRROR_FROM_PORT_DIR) $(MIRROR_FROM_REP_DIR) LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/yquake2)

$(MIRROR_FROM_PORT_DIR):
	mkdir -p $(dir $@)
	cp -r $(PORT_DIR)/$@ $(dir $@)
	$(mirror_from_rep_dir)

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/app/yquake2/LICENSE $@

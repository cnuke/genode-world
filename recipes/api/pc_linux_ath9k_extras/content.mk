#
# Content hosted in the pc repository
#

MIRRORED_FROM_REP_DIR := lib/mk/pc_lx_ath9k_extras.mk \
                         lib/import/import-pc_lx_ath9k_extras.mk

content: $(MIRRORED_FROM_REP_DIR)
$(MIRRORED_FROM_REP_DIR):
	$(mirror_from_rep_dir)

#
# Content from the Linux source tree
#

PORT_DIR := $(call port_dir,$(GENODE_DIR)/repos/dde_linux/ports/linux)
LX_REL_DIR := src/linux_extras
LX_REL_SRC_DIR := src/linux
LX_ABS_DIR := $(addsuffix /$(LX_REL_SRC_DIR),$(PORT_DIR))

# add content listed in the repository's source.list or dep.list files
LX_FILE_LISTS := $(shell find -H $(REP_DIR) -name dep.list -or -name source.list)
LX_FILES += $(shell cat $(LX_FILE_LISTS))
LX_FILES := $(sort $(LX_FILES))
MIRRORED_FROM_PORT_DIR += $(LX_FILES)

content: $(MIRRORED_FROM_PORT_DIR)
$(MIRRORED_FROM_PORT_DIR):
	mkdir -p $(dir $(addprefix $(LX_REL_DIR)/,$@))
	cp -r $(addprefix $(LX_ABS_DIR)/,$@) $(addprefix $(LX_REL_DIR)/,$@)

content: LICENSE
LICENSE:
	cp $(PORT_DIR)/src/linux/COPYING $@

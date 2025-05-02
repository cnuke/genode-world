MIRROR_FROM_REP_DIR = lib/import/import-opusfile.mk lib/mk/opusfile.mk

content: $(MIRROR_FROM_REP_DIR) src/lib/opusfile LICENSE

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/opusfile)

src/lib/opusfile:
	mkdir -p src/lib/opusfile
	cp -r $(PORT_DIR)/src/lib/opusfile/* src/lib/opusfile

LICENSE:
	cp $(PORT_DIR)/src/lib/opusfile/COPYING $@

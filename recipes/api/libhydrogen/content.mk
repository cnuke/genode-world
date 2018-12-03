content: include lib/symbols/libhydrogen LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/libhydrogen)

include:
	mkdir -p $@
	cp -r $(PORT_DIR)/include/libhydrogen/* $@

lib/symbols/libhydrogen:
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/libhydrogen/LICENSE $@

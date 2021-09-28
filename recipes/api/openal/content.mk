content: include lib/symbols/openal LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/openal)

include:
	mkdir -p $@
	cp -r $(PORT_DIR)/include/* $@

lib/symbols/openal:
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/openal/COPYING $@

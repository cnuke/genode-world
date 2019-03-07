content: include lib/symbols/libsodium LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/libsodium)

include:
	mkdir -p $@
	cp -r $(REP_DIR)/include/libsodium/* $@
	cp -r $(PORT_DIR)/include/libsodium/* $@

lib/symbols/libsodium:
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/libsodium/LICENSE $@

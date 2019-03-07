MIRROR_FROM_REP_DIR = lib/import/import-libsodium.mk lib/mk/libsodium.mk

content: $(MIRROR_FROM_REP_DIR) src/lib/libsodium/target.mk LICENSE

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/libsodium)

src/lib/libsodium/target.mk:
	mkdir -p src/lib/libsodium
	cp -r $(PORT_DIR)/src/lib/libsodium/* src/lib/libsodium
	cp -r $(REP_DIR)/src/lib/libsodium/* src/lib/libsodium
	echo "LIBS := libsodium" > $@

LICENSE:
	cp $(PORT_DIR)/src/lib/libsodium/LICENSE $@

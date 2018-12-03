MIRROR_FROM_REP_DIR = lib/import/import-libhydrogen.mk lib/mk/libhydrogen.mk

content: $(MIRROR_FROM_REP_DIR) src/lib/libhydrogen/target.mk LICENSE

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/libhydrogen)

src/lib/libhydrogen/target.mk:
	mkdir -p src/lib/libhydrogen
	cp -r $(PORT_DIR)/src/lib/libhydrogen/* src/lib/libhydrogen
	echo "LIBS := libhydrogen" > $@

LICENSE:
	cp $(PORT_DIR)/src/lib/libhydrogen/LICENSE $@

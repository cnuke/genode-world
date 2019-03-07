content: src/app/minisign LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/minisign)

src/app/minisign:
	mkdir -p $@
	cp -r $(PORT_DIR)/src/app/minisign/* $@
	cp -r $(REP_DIR)/src/app/minisign/* $@

LICENSE:
	cp $(PORT_DIR)/src/app/minisign/LICENSE $@

content: src/app/encpipe LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/encpipe)

src/app/encpipe:
	mkdir -p $@
	cp -r $(PORT_DIR)/src/app/encpipe/* $@
	cp -r $(REP_DIR)/src/app/encpipe/* $@

LICENSE:
	cp $(PORT_DIR)/src/app/encpipe/LICENSE $@

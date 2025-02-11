MIRROR_FROM_REP_DIR := lib/symbols/glib
content: include $(MIRROR_FROM_REP_DIR) LICENSE glib-2.0.pc

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/glib)

include:
	mkdir -p $@
	cp -r $(PORT_DIR)/include/glib/* $@
	cp -r $(REP_DIR)/include/glib/* $@

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

LICENSE:
	cp $(PORT_DIR)/src/lib/glib/COPYING $@

VERSION := $(shell sed -n 's/VERSION.*:=[ ]*\(.*\)/\1/p' $(REP_DIR)/ports/glib.port)

glib-2.0.pc:
	echo "glib_genmarshal=glib-genmarshal" > $@
	echo "gobject_query=gobject-query" >> $@
	echo "glib_mkenums=glib-mkenums" >> $@
	echo "Name: Glib" >> $@
	echo "Description: C Utility Library" >> $@
	echo "Version: $(VERSION)" >> $@
	echo "Libs: -l:glib.lib.so" >> $@

content: src/lib/openal/target.mk lib/mk LICENSE

PORT_DIR := $(call port_dir,$(REP_DIR)/ports/openal)

src/lib/openal:
	mkdir -p $@
	cp -r $(PORT_DIR)/src/lib/openal/hrtf_default.h $@
	cp -r $(PORT_DIR)/src/lib/openal/common         $@
	cp -r $(PORT_DIR)/src/lib/openal/core           $@
	cp -r $(PORT_DIR)/src/lib/openal/alc            $@
	cp -r $(PORT_DIR)/src/lib/openal/al             $@
	cp -r $(REP_DIR)/src/lib/openal/*               $@

src/lib/openal/target.mk: src/lib/openal
	echo "LIBS += openal" > $@

lib/mk:
	mkdir -p $@
	cp $(REP_DIR)/$@/openal.mk $@

LICENSE:
	cp $(PORT_DIR)/src/lib/openal/COPYING $@

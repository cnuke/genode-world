content: src/app/linphone-simple LICENSE

src/app/linphone-simple:
	mkdir -p $@
	cp -r $(REP_DIR)/src/app/linphone-simple/* $@/

LICENSE:
	cp $(REP_DIR)/src/app/linphone-simple/LICENSE $@

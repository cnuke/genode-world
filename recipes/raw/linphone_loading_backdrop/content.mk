content: backdrop.config

backdrop.config:
	cp $(REP_DIR)/recipes/raw/linphone_loading_backdrop/$@ $@

content: linphone_logo.png

linphone_logo.png:
	cp $(REP_DIR)/recipes/raw/linphone_loading_backdrop/$@ $@

include ports/uhexen2.inc

UHEXEN2         = hexen2source-$(UHEXEN2_VERSION)
UHEXEN2_TGZ     = $(UHEXEN2).tgz
UHEXEN2_URL     = http://downloads.sourceforge.net/project/uhexen2/Hammer%20of%20Thyrion/$(UHEXEN2_VERSION)/Source/$(UHEXEN2_TGZ)

PORTS += uhexen2-$(UHEXEN2_VERSION)

prepare:: $(CONTRIB_DIR)/$(UHEXEN2)

$(DOWNLOAD_DIR)/$(UHEXEN2_TGZ):
	$(VERBOSE)wget -c $(UHEXEN2_URL) -O $(DOWNLOAD_DIR)/$(UHEXEN2_TGZ) \
		&& touch $@

$(CONTRIB_DIR)/$(UHEXEN2): $(DOWNLOAD_DIR)/$(UHEXEN2_TGZ)
	$(VERBOSE)tar xfz $< -C $(CONTRIB_DIR) && touch $@

clean-uhexen2:
	$(VERBOSE)rm -rf $(CONTRIB_DIR)/$(UHEXEN2)

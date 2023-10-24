TARGET := linphone-simple-qml-dummy

LP_QML_TAR         := $(BUILD_BASE_DIR)/bin/linphone-simple_qml.tar
LP_QML_ARCHIVE_DIR := linphone-simple

$(TARGET): $(LP_QML_TAR)

remove_stale:
	$(VERBOSE)rm $(LP_QML_TAR) || true

$(LP_QML_TAR): remove_stale
	$(VERBOSE)mkdir -p $(LP_QML_ARCHIVE_DIR)
	$(VERBOSE)cp -r $(PRG_DIR)/assets $(LP_QML_ARCHIVE_DIR)
	$(VERBOSE)cp -r $(PRG_DIR)/qml $(LP_QML_ARCHIVE_DIR)
	$(VERBOSE)cp -r $(PRG_DIR)/ringtones $(LP_QML_ARCHIVE_DIR)

	$(VERBOSE)tar chf $@ $(LP_QML_ARCHIVE_DIR)

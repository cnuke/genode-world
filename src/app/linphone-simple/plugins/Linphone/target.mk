QMAKE_PROJECT_FILE = $(PRG_DIR)/linphone.pro

QT5_PORT_LIBS += libQt5Core libQt5Gui libQt5Network
QT5_PORT_LIBS += libQt5Qml libQt5QmlModels libQt5Quick

LIBS = libc libm mesa qt5_component stdcxx $(QT5_PORT_LIBS)

include $(call select_from_repositories,lib/import/import-qt5_qmake.mk)

LP_PLUGIN_TAR := $(BUILD_BASE_DIR)/bin/linphone-simple-linphone-plugin.tar
LP_PLUGIN_SO  := liblinphoneplugin.lib.so

LP_ARCHIVE_DIR := qt/qml/Linphone
LP_IMPORTS_DIR := imports/Linphone

$(TARGET): $(LP_PLUGIN_TAR)

$(LP_PLUGIN_TAR): build_with_qmake
	$(VERBOSE)mkdir -p $(LP_ARCHIVE_DIR)
	$(VERBOSE)cp $(PRG_DIR)/qmldir $(LP_ARCHIVE_DIR)
	$(VERBOSE)$(STRIP) $(CURDIR)/$(LP_IMPORTS_DIR)/$(LP_PLUGIN_SO) \
		-o $(CURDIR)/$(LP_IMPORTS_DIR)/$(LP_PLUGIN_SO).stripped
	$(VERBOSE)cp $(LP_IMPORTS_DIR)/$(LP_PLUGIN_SO).stripped \
		$(LP_ARCHIVE_DIR)/$(LP_PLUGIN_SO)
	$(VERBOSE)ln -sf $(CURDIR)/$(LP_IMPORTS_DIR)/$(LP_PLUGIN_SO).stripped \
		$(BUILD_BASE_DIR)/bin/$(LP_PLUGIN_SO)
	$(VERBOSE)tar chf $@ qt

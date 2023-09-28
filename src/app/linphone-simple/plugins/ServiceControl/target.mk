QMAKE_PROJECT_FILE = $(PRG_DIR)/servicecontrol.pro

QT5_PORT_LIBS += libQt5Core libQt5Gui libQt5Network
QT5_PORT_LIBS += libQt5Qml libQt5QmlModels libQt5Quick

LIBS = libc libm mesa qt5_component stdcxx $(QT5_PORT_LIBS)

include $(call select_from_repositories,lib/import/import-qt5_qmake.mk)

SC_PLUGIN_TAR := $(BUILD_BASE_DIR)/bin/linphone-simple-servicecontrol-plugin.tar
SC_PLUGIN_SO  := libservicecontrolplugin.lib.so

SC_ARCHIVE_DIR := qt/qml/ServiceControl
SC_IMPORTS_DIR := imports/ServiceControl

$(TARGET): $(SC_PLUGIN_TAR)

$(SC_PLUGIN_TAR): build_with_qmake
	$(VERBOSE)mkdir -p $(SC_ARCHIVE_DIR)
	$(VERBOSE)cp $(PRG_DIR)/qmldir $(SC_ARCHIVE_DIR)
	$(VERBOSE)$(STRIP) $(CURDIR)/$(SC_IMPORTS_DIR)/$(SC_PLUGIN_SO) \
		-o $(CURDIR)/$(SC_IMPORTS_DIR)/$(SC_PLUGIN_SO).stripped
	$(VERBOSE)cp $(SC_IMPORTS_DIR)/$(SC_PLUGIN_SO).stripped \
		$(SC_ARCHIVE_DIR)/$(SC_PLUGIN_SO)
	$(VERBOSE)ln -sf $(CURDIR)/$(SC_IMPORTS_DIR)/$(SC_PLUGIN_SO).stripped \
		$(BUILD_BASE_DIR)/bin/$(SC_PLUGIN_SO)
	$(VERBOSE)tar chf $@ qt

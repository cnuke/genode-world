TEMPLATE = lib
CONFIG += qt plugin qmltypes
QT += qml quick quick-private

QML_IMPORT_NAME = ServiceControl
QML_IMPORT_MAJOR_VERSION = 1

DESTDIR = imports/ServiceControl
TARGET  = servicecontrolplugin

SOURCES += plugin.cpp servicecontrol.cpp

HEADERS += servicecontrol.h

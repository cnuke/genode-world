TEMPLATE = lib
CONFIG += qt plugin qmltypes
QT += qml quick quick-private

QML_IMPORT_NAME = Linphone
QML_IMPORT_MAJOR_VERSION = 1

DESTDIR = imports/Linphone
TARGET  = linphoneplugin

SOURCES += plugin.cpp linphone.cpp

HEADERS += linphone.h

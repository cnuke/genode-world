# identify the qt5 repository by searching for a file that is unique for qt5
QT5_REP_DIR := $(call select_from_repositories,lib/import/import-qt5.inc)
QT5_REP_DIR := $(realpath $(dir $(QT5_REP_DIR))../..)

include $(QT5_REP_DIR)/src/app/qt5/tmpl/target_defaults.inc

include $(QT5_REP_DIR)/src/app/qt5/tmpl/target_final.inc

LIBS += qoost qt5_component

agent.o:         agent.moc
control_panel.o: control_panel.moc
platform.o:      platform.moc

# XXX manually moc parts of qoost
SRC_CC += moc_icon.cpp
moc_icon.cpp: icon.h

vpath icon.h $(call select_from_ports,qoost)/include/qoost

CC_CXX_WARN_STRICT :=

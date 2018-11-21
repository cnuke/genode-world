CMUS_DIR := $(call select_from_ports,cmus)/src/app/cmus

SRC_C := op/sndio.c debug.c

INC_DIR := $(CMUS_DIR)

LIBS := libc libsndio

SHARED_LIB := yes

vpath %.c $(CMUS_DIR)

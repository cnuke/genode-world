TARGET := encpipe

ENCPIPE_SRC_DIR := $(call select_from_ports,encpipe)/src/app/encpipe/src

SRC_C := \
         encpipe.c \
         safe_rw.c

INC_DIR += $(ENCPIPE_SRC_DIR)

LIBS := libc libhydrogen posix

vpath %.c $(ENCPIPE_SRC_DIR)

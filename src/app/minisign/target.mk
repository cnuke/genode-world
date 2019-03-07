TARGET := minisign

MINISIGN_SRC_DIR := $(call select_from_ports,minisign)/src/app/minisign/src

SRC_C := \
         base64.c \
         helpers.c \
         get_line.c \
         minisign.c

INC_DIR += $(MINISIGN_SRC_DIR)

LIBS := libc libsodium posix

vpath %.c $(MINISIGN_SRC_DIR)

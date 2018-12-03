include $(REP_DIR)/lib/import/import-libhydrogen.mk

LIBHYDROGEN_SRC_DIR := $(LIBHYDROGEN_PORT_DIR)/src/lib/libhydrogen

SRC_C := hydrogen.c

INC_DIR += $(LIBHYDROGEN_SRC_DIR)

CC_DEF += -D__unix__ -DTLS=

LIBS := libc

vpath %.c $(LIBHYDROGEN_SRC_DIR)

SHARED_LIB := yes

CC_CXX_WARN_STRICT :=

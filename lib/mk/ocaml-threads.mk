include $(REP_DIR)/lib/mk/ocaml.inc

LIBS += libc

INC_DIR += $(OCAML_SRC_DIR)/byterun
INC_DIR += $(OCAML_SRC_DIR)/byterun/caml

vpath %.c $(OCAML_SRC_DIR)/otherlibs/threads

SRC_C += scheduler.c

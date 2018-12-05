CMUS_DIR := $(call select_from_ports,cmus)/src/app/cmus

SRC_C := ip/mad.c ip/nomad.c

# XXX remove later
SRC_C += xmalloc.c debug.c keyval.c read_wrapper.c file.c comment.c \
         uchar.c gbuf.c convert.c channelmap.c id3.c ape.c misc.c prog.c \
         xstrjoin.c

INC_DIR := $(CMUS_DIR)
INC_DIR += $(REP_DIR)/src/app/cmus

LIBS := libc libmad

SHARED_LIB := yes

vpath %.c $(CMUS_DIR)

DISCOUNT_DIR = $(call select_from_ports,discount)/src/noux-pkg/discount

TARGET := discount

# libmarkdown
SRC_C := Csio.c \
         basename.c \
         css.c \
         docheader.c \
         dumptree.c \
         emmatch.c \
         flags.c \
         generate.c \
         github_flavoured.c \
         html5.c \
         markdown.c \
         mkdio.c \
         resource.c \
         setup.c \
         tags.c \
         toc.c \
         version.c \
         xml.c \
         xmlpage.c

# markdown
SRC_C += main.c \
         pgm_options.c

INC_DIR += $(DISCOUNT_DIR) \
           $(REP_DIR)/src/noux-pkg/discount

# highway to the danger zone
CC_WARN=

LIBS = libc libc_noux

vpath % $(DISCOUNT_DIR)

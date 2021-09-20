YQUAKE2_DIR := $(call select_from_ports,yquake2)/src/app/yquake2

LIBS       := libc libm sdl2
SHARED_LIB := yes

INC_DIR := $(YQUAKE2_DIR)

CC_C_OPT := -std=gnu99 -fno-strict-aliasing -fwrapv
#-fvisibility=hidden
#CC_C_OPT += -mfpmath=sse
CC_C_OPT += -ffp-contract=off

CC_C_OPT += -DYQ2OSTYPE=\"Genode\" -DYQ2ARCH=\"x86_64\"

SRC_C := \
	src/client/refresh/soft/sw_aclip.c \
	src/client/refresh/soft/sw_alias.c \
	src/client/refresh/soft/sw_bsp.c \
	src/client/refresh/soft/sw_draw.c \
	src/client/refresh/soft/sw_edge.c \
	src/client/refresh/soft/sw_image.c \
	src/client/refresh/soft/sw_light.c \
	src/client/refresh/soft/sw_main.c \
	src/client/refresh/soft/sw_misc.c \
	src/client/refresh/soft/sw_model.c \
	src/client/refresh/soft/sw_part.c \
	src/client/refresh/soft/sw_poly.c \
	src/client/refresh/soft/sw_polyset.c \
	src/client/refresh/soft/sw_rast.c \
	src/client/refresh/soft/sw_scan.c \
	src/client/refresh/soft/sw_sprite.c \
	src/client/refresh/soft/sw_surf.c \
	src/client/refresh/files/pcx.c \
	src/client/refresh/files/stb.c \
	src/client/refresh/files/wal.c \
	src/client/refresh/files/pvs.c \
	src/common/shared/shared.c \
	src/common/md4.c

SRC_C += src/backends/unix/shared/hunk.c

vpath %.c $(YQUAKE2_DIR)

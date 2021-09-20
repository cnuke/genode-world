YQUAKE2_DIR := $(call select_from_ports,yquake2)/src/app/yquake2

LIBS       := libc libm mesa sdl2
SHARED_LIB := yes

INC_DIR := $(YQUAKE2_DIR)

CC_C_OPT := -std=gnu99 -fno-strict-aliasing -fwrapv
#-fvisibility=hidden
#CC_C_OPT += -mfpmath=sse
CC_C_OPT += -ffp-contract=off

CC_C_OPT += -DYQ2OSTYPE=\"Genode\" -DYQ2ARCH=\"x86_64\"

SRC_C := \
	src/client/refresh/gl1/qgl.c \
	src/client/refresh/gl1/gl1_draw.c \
	src/client/refresh/gl1/gl1_image.c \
	src/client/refresh/gl1/gl1_light.c \
	src/client/refresh/gl1/gl1_lightmap.c \
	src/client/refresh/gl1/gl1_main.c \
	src/client/refresh/gl1/gl1_mesh.c \
	src/client/refresh/gl1/gl1_misc.c \
	src/client/refresh/gl1/gl1_model.c \
	src/client/refresh/gl1/gl1_scrap.c \
	src/client/refresh/gl1/gl1_surf.c \
	src/client/refresh/gl1/gl1_warp.c \
	src/client/refresh/gl1/gl1_sdl.c \
	src/client/refresh/gl1/gl1_md2.c \
	src/client/refresh/gl1/gl1_sp2.c \
	src/client/refresh/files/pcx.c \
	src/client/refresh/files/stb.c \
	src/client/refresh/files/wal.c \
	src/client/refresh/files/pvs.c \
	src/common/shared/shared.c \
	src/common/md4.c

SRC_C += src/backends/unix/shared/hunk.c

vpath %.c $(YQUAKE2_DIR)

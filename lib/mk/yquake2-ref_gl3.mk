YQUAKE2_DIR := $(call select_from_ports,yquake2)/src/app/yquake2

LIBS       := libc libm mesa sdl2
SHARED_LIB := yes

INC_DIR := $(YQUAKE2_DIR) \
           $(YQUAKE2_DIR)/src/client/refresh/gl3/glad/include

CC_C_OPT := -std=gnu99 -fno-strict-aliasing -fwrapv
#-fvisibility=hidden
#CC_C_OPT += -mfpmath=sse
CC_C_OPT += -ffp-contract=off

CC_C_OPT += -DYQ2OSTYPE=\"Genode\" -DYQ2ARCH=\"x86_64\"

SRC_C := \
	src/client/refresh/gl3/gl3_draw.c \
	src/client/refresh/gl3/gl3_image.c \
	src/client/refresh/gl3/gl3_light.c \
	src/client/refresh/gl3/gl3_lightmap.c \
	src/client/refresh/gl3/gl3_main.c \
	src/client/refresh/gl3/gl3_mesh.c \
	src/client/refresh/gl3/gl3_misc.c \
	src/client/refresh/gl3/gl3_model.c \
	src/client/refresh/gl3/gl3_sdl.c \
	src/client/refresh/gl3/gl3_surf.c \
	src/client/refresh/gl3/gl3_warp.c \
	src/client/refresh/gl3/gl3_shaders.c \
	src/client/refresh/gl3/gl3_md2.c \
	src/client/refresh/gl3/gl3_sp2.c \
	src/client/refresh/gl3/glad/src/glad.c \
	src/client/refresh/files/pcx.c \
	src/client/refresh/files/stb.c \
	src/client/refresh/files/wal.c \
	src/client/refresh/files/pvs.c \
	src/common/shared/shared.c \
	src/common/md4.c


SRC_C += src/backends/unix/shared/hunk.c

vpath %.c $(YQUAKE2_DIR)

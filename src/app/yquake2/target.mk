TARGET := yquake2

LIBS := base libc libm curl sdl2

YQUAKE2_DIR := $(call select_from_ports,yquake2)/src/app/yquake2

CC_C_OPT := -std=gnu99 -fno-strict-aliasing -fwrapv
#-fvisibility=hidden
#CC_C_OPT += -mfpmath=sse
CC_C_OPT += -ffp-contract=off

CC_C_OPT += -DYQ2OSTYPE=\"Genode\" -DYQ2ARCH=\"x86_64\"
CC_C_OPT += -D__Genode__

SRC_C := \
	src/backends/generic/misc.c \
	src/client/cl_cin.c \
	src/client/cl_console.c \
	src/client/cl_download.c \
	src/client/cl_effects.c \
	src/client/cl_entities.c \
	src/client/cl_input.c \
	src/client/cl_inventory.c \
	src/client/cl_keyboard.c \
	src/client/cl_lights.c \
	src/client/cl_main.c \
	src/client/cl_network.c \
	src/client/cl_parse.c \
	src/client/cl_particles.c \
	src/client/cl_prediction.c \
	src/client/cl_screen.c \
	src/client/cl_tempentities.c \
	src/client/cl_view.c \
	src/client/curl/download.c \
	src/client/curl/qcurl.c \
	src/client/input/sdl.c \
	src/client/menu/menu.c \
	src/client/menu/qmenu.c \
	src/client/menu/videomenu.c \
	src/client/sound/sdl.c \
	src/client/sound/ogg.c \
	src/client/sound/openal.c \
	src/client/sound/qal.c \
	src/client/sound/sound.c \
	src/client/sound/wave.c \
	src/client/vid/glimp_sdl.c \
	src/client/vid/vid.c \
	src/common/argproc.c \
	src/common/clientserver.c \
	src/common/collision.c \
	src/common/crc.c \
	src/common/cmdparser.c \
	src/common/cvar.c \
	src/common/filesystem.c \
	src/common/glob.c \
	src/common/md4.c \
	src/common/movemsg.c \
	src/common/frame.c \
	src/common/netchan.c \
	src/common/pmove.c \
	src/common/szone.c \
	src/common/zone.c \
	src/common/shared/flash.c \
	src/common/shared/rand.c \
	src/common/shared/shared.c \
	src/common/unzip/ioapi.c \
	src/common/unzip/miniz.c \
	src/common/unzip/unzip.c \
	src/server/sv_cmd.c \
	src/server/sv_conless.c \
	src/server/sv_entities.c \
	src/server/sv_game.c \
	src/server/sv_init.c \
	src/server/sv_main.c \
	src/server/sv_save.c \
	src/server/sv_send.c \
	src/server/sv_user.c \
	src/server/sv_world.c

SRC_C += \
	src/backends/unix/main.c \
	src/backends/unix/network.c \
	src/backends/unix/signalhandler.c \
	src/backends/unix/system.c \
	src/backends/unix/shared/hunk.c

vpath %.c $(YQUAKE2_DIR)

SDL2_PORT_DIR := $(call select_from_ports,sdl2)
SDL2_DIR      := $(SDL2_PORT_DIR)/src/lib/sdl2

# build shared object
SHARED_LIB = yes

# use default warning level for 3rd-party code
CC_WARN =

CC_OPT += -DGENODE

# backends
SRC_CC   = video/SDL_genode_fb_video.cc \
           video/SDL_genode_fb_events.cc \
           audio/SDL_genodeaudio.cc \
           timer/SDL_systimer.cc \
           loadso/SDL_loadso.cc

INC_DIR += $(REP_DIR)/include/SDL \
           $(REP_DIR)/src/lib/sdl \
           $(REP_DIR)/src/lib/sdl/thread \
           $(REP_DIR)/src/lib/sdl/video

# main files
SRC_C    = SDL.c \
           SDL_error.c \
           SDL_fatal.c \
           SDL_hints.c \
           SDL_log.c
INC_DIR += $(SDL2_DIR)/src

# atomic subsystem
SRC_C += $(addprefix atomic/,$(notdir $(wildcard $(SDL2_DIR)/src/atomic/*.c)))

# audio subsystem
SRC_C += $(addprefix audio/,$(notdir $(wildcard $(SDL2_DIR)/src/audio/*.c)))
INC_DIR += $(SDL2_DIR)/src/audio

# cpuinfo subsystem
SRC_C   += cpuinfo/SDL_cpuinfo.c

# dynapi subsystem
SRC_C   += dynapi/SDL_dynapi.c

# event subsystem
SRC_C += $(addprefix events/,$(notdir $(wildcard $(SDL2_DIR)/src/events/*.c)))
INC_DIR += $(SDL2_DIR)/src/events

# file I/O subsystem
SRC_C   += file/SDL_rwops.c

# filesystem subsystem
SRC_C   += file/unix/SDL_sysfilesystem.c

# haptic subsystem
SRC_C   += haptic/SDL_haptic.c \
           haptic/dummy/SDL_syshaptic.c
INC_DIR += $(SDL2_DIR)/src/haptic

# joystick subsystem
SRC_C   += joystick/SDL_joystick.c \
           joystick/SDL_gamecontroller.c \
           joystick/dummy/SDL_sysjoystick.c
INC_DIR += $(SDL2_DIR)/src/joystick

# render subsystem
SRC_C   += $(addprefix render/,$(notdir $(wildcard $(SDL2_DIR)/src/render/*.c)))
SRC_C   += $(addprefix render/,$(notdir $(wildcard $(SDL2_DIR)/src/render/software/*.c)))
INC_DIR += $(SDL2_DIR)/src/render $(SDL2_DIR)/src/render/software

# stdlib files
SRC_C   += stdlib/SDL_getenv.c \
           stdlib/SDL_string.h

# thread subsystem
SRC_C   += thread/SDL_thread.c \
           thread/generic/SDL_syscond.c \
           thread/generic/SDL_sysmutex.c \
           thread/generic/SDL_systls.c \
           thread/pthread/SDL_syssem.c \
           thread/pthread/SDL_systhread.c \
INC_DIR += $(SDL2_DIR)/src/thread

# timer subsystem
SRC_C   += timer/SDL_timer.c
INC_DIR += $(SDL2_DIR)/src/timer

# video subsystem
SRC_C += $(addprefix video/,$(notdir $(wildcard $(SDL2_DIR)/src/video/*.c)))
INC_DIR += $(SDL2_DIR)/src/video

# we need libc
LIBS = libc pthread sdl2_api

# backend path
vpath % $(REP_DIR)/src/lib/sdl2

vpath % $(SDL2_DIR)/src

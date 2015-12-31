TARGET := dgen-sdl

DGEN_SDL_DIR := $(call select_from_ports,dgen-sdl)/src/app/dgen-sdl

SRC_CC := $(notdir $(wildcard $(REP_DIR)/src/app/dgen-sdl/*.cpp))
SRC_C  :=

INC_DIR += $(DGEN_SDL_DIR) $(REP_DIR)/src/app/dgen-sdl

# cz80
SRC_C   += cz80.c
INC_DIR += $(DGEN_SDL_DIR)/cz80

# sdl (dgenfont files are generate while compiling)
SRC_CC  += $(notdir $(wildcard $(DGEN_SDL_DIR)/sdl/*.cpp))
SRC_C   += prompt.c
INC_DIR += $(DGEN_SDL_DIR)/sdl

# musa
FILTER_OUT_musa := m68k_in.c m68kmake.c
SRC_C   += $(filter-out $(FILTER_OUT_musa),$(notdir $(wildcard $(DGEN_SDL_DIR)/musa/*.c)))
INC_DIR += $(DGEN_SDL_DIR)/musa

# mz80.c is normally generate by makez80 while compiling dgen-sdl
SRC_C   += mz80.c

# m68kops.c is normally generate by m68make while compiling dgen-sdl
SRC_C   += m68kops.c

# scale2x
SRC_C   += $(notdir $(wildcard $(DGEN_SDL_DIR)/scale2x/*.c))
INC_DIR += $(DGEN_SDL_DIR)/scale2x

# hqx
SRC_C   +=  $(filter-out hqx.c,$(notdir $(wildcard $(DGEN_SDL_DIR)/hqx/src/*.c)))
INC_DIR += $(DGEN_SDL_DIR)/hqx/src

# main
SRC_CC  += rc.cpp    \
           md.cpp    \
           mdfr.cpp  \
           vdp.cpp   \
           save.cpp  \
           graph.cpp \
           myfm.cpp  \
           ras.cpp   \
           main.cpp  \
           mem.cpp

SRC_C   += ckvp.c     \
           decode.c   \
           fm.c       \
           romload.c  \
           sn76496.c  \
           system.c

CC_OPT += -DPACKAGE_NAME=\"DGen/SDL\" -DPACKAGE_TARNAME=\"dgen-sdl\"       \
          -DPACKAGE_VERSION=\"1.33\" -DPACKAGE_STRING=\"DGen/SDL\ 1.33\"   \
          -DPACKAGE_BUGREPORT=\"zamaz@users.sourceforge.net\"              \
          -DPACKAGE_URL=\"http://sourceforge.net/projects/dgen\"           \
          -DPACKAGE=\"dgen-sdl\" -DVERSION=\"1.33\" -DSTDC_HEADERS=1       \
          -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1       \
          -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1           \
          -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1          \
          -DHAVE_SDL_WM_TOGGLEFULLSCREEN=1 -DHAVE_FTELLO=1 -DHAVE_GLOB_H=1 \
          -DWITH_MUSA=1 -DWITH_MZ80=1 -DWITH_CZ80=1                        \
          -DWITH_CTV=1 -DWITH_HQX=1 -DWITH_SCALE2X=1

# remove for debug messages
CC_OPT += -DNDEBUG=1

# scale2x uses restrict keyword
CC_C_OPT += -std=c99

LIBS += config_args libc libm sdl stdcxx zlib


vpath % $(REP_DIR)/src/app/dgen-sdl
vpath % $(DGEN_SDL_DIR)
vpath % $(DGEN_SDL_DIR)/cz80
vpath % $(DGEN_SDL_DIR)/hqx/src
vpath % $(DGEN_SDL_DIR)/scale2x
vpath % $(DGEN_SDL_DIR)/sdl
vpath % $(DGEN_SDL_DIR)/musa

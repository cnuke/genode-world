YQUAKE2_DIR := $(call select_from_ports,yquake2)/src/app/yquake2

LIBS       := libc libm
SHARED_LIB := yes

INC_DIR := $(YQUAKE2_DIR)

CC_C_OPT := -std=gnu99 -fno-strict-aliasing -fwrapv
#-fvisibility=hidden
# CC_C_OPT += -mfpmath=sse
CC_C_OPT += -ffp-contract=off

CC_C_OPT += -DYQ2OSTYPE=\"Genode\" -DYQ2ARCH=\"x86_64\"

SRC_C := \
        src/common/shared/flash.c \
        src/common/shared/rand.c \
        src/common/shared/shared.c \
        src/game/g_ai.c \
        src/game/g_chase.c \
        src/game/g_cmds.c \
        src/game/g_combat.c \
        src/game/g_func.c \
        src/game/g_items.c \
        src/game/g_main.c \
        src/game/g_misc.c \
        src/game/g_monster.c \
        src/game/g_phys.c \
        src/game/g_spawn.c \
        src/game/g_svcmds.c \
        src/game/g_target.c \
        src/game/g_trigger.c \
        src/game/g_turret.c \
        src/game/g_utils.c \
        src/game/g_weapon.c \
        src/game/monster/berserker/berserker.c \
        src/game/monster/boss2/boss2.c \
        src/game/monster/boss3/boss3.c \
        src/game/monster/boss3/boss31.c \
        src/game/monster/boss3/boss32.c \
        src/game/monster/brain/brain.c \
        src/game/monster/chick/chick.c \
        src/game/monster/flipper/flipper.c \
        src/game/monster/float/float.c \
        src/game/monster/flyer/flyer.c \
        src/game/monster/gladiator/gladiator.c \
        src/game/monster/gunner/gunner.c \
        src/game/monster/hover/hover.c \
        src/game/monster/infantry/infantry.c \
        src/game/monster/insane/insane.c \
        src/game/monster/medic/medic.c \
        src/game/monster/misc/move.c \
        src/game/monster/mutant/mutant.c \
        src/game/monster/parasite/parasite.c \
        src/game/monster/soldier/soldier.c \
        src/game/monster/supertank/supertank.c \
        src/game/monster/tank/tank.c \
        src/game/player/client.c \
        src/game/player/hud.c \
        src/game/player/trail.c \
        src/game/player/view.c \
        src/game/player/weapon.c \
        src/game/savegame/savegame.c

vpath %.c $(YQUAKE2_DIR)

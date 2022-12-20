REQUIRES := x86

TARGET_LIB_DIR := $(REP_DIR)/src/lib/wifi_ath9k

SHARED_LIB := yes

LD_OPT += --version-script=$(TARGET_LIB_DIR)/symbol.map

LIBS    += base jitterentropy pc_linux_generated pc_lx_emul pc_lx_ath9k_extras
INC_DIR := $(TARGET_LIB_DIR)
SRC_CC  += wlan.cc
SRC_CC  += wlan_usb.cc
SRC_CC  += misc.cc
SRC_CC  += time.cc
SRC_CC  += firmware.cc
SRC_CC  += socket_call.cc
SRC_CC  += random.cc

SRC_C   += dummies.c
SRC_C   += lx_emul.c
SRC_C   += lx_user.c
SRC_C   += uplink.c
SRC_C   += lx_usb_wrapper.c

CC_OPT_lx_emul/shadow/kernel/softirq += -DKBUILD_MODNAME='"shadow_softirq"'
SRC_C   += lx_emul/shadow/kernel/softirq.c

CC_OPT_lx_socket_call += -DKBUILD_MODNAME='"lx_socket_call"'
SRC_C   += lx_socket_call.c

SRC_C   += $(notdir $(wildcard $(TARGET_LIB_DIR)/generated_dummies.c))
SRC_C   += $(notdir $(wildcard $(TARGET_LIB_DIR)/extra_dummies.c))
SRC_C   += lx_emul/common_dummies.c

CC_C_OPT += -I$(LX_SRC_DIR)/drivers/net/wireless/intel/iwlwifi
CC_C_OPT += -I$(LX_SRC_DIR)/include/linux

CC_C_OPT += -Wno-address-of-packed-member

# need net/rfkill/rfkill.h
CC_OPT_lx_emul += -I$(LX_SRC_DIR)

CC_C_OPT += -DCONFIG_RFKILL_INPUT

CC_OPT += -DCONFIG_IWLWIFI_DEBUG

# need to look in the pc repository or in our own in case of depot builds
vpath %.c  $(REP_DIR)/../pc/src/lib/pc
vpath %.cc $(REP_DIR)/../pc/src/lib/pc

vpath %.c  $(REP_DIR)/src/lib/pc
vpath %.cc $(REP_DIR)/src/lib/pc

vpath %.c $(TARGET_LIB_DIR)
vpath %.cc $(TARGET_LIB_DIR)

CUSTOM_TARGET_DEPS += $(TARGET_LIB_DIR)/symbol.map

#
# Genode C-API backends
#

SRC_CC  += genode_c_api/uplink.cc

vpath genode_c_api/uplink.cc $(subst /genode_c_api,,$(call select_from_repositories,src/lib/genode_c_api))

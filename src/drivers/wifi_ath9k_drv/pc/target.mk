TARGET  := wifi_ath9k_drv
SRC_CC  := main.cc wpa.cc
LIBS    := base wifi_ath9k wifi_firmware
LIBS    += libc
LIBS    += wpa_supplicant
LIBS    += libcrypto libssl wpa_driver_nl80211

INC_DIR += $(PRG_DIR)

CC_CXX_WARN_STRICT :=

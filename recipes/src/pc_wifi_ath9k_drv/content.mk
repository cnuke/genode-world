#
# Driver portions
#

# PC_LIB_MK := $(addprefix lib/mk/,wifi_firmware.mk)

LIB_MK := $(addprefix lib/mk/,wifi_ath9k.inc vfs_wifi_ath9k.mk) \
          $(foreach SPEC,x86_32 x86_64,lib/mk/spec/$(SPEC)/wifi_ath9k.mk)

MIRROR_FROM_REP_DIR := src/drivers/wifi_ath9k_drv \
                       $(shell cd $(REP_DIR); find src/lib/wifi_ath9k -type f) \
					   $(shell cd $(REP_DIR); find src/lib/vfs/wifi_ath9k -type f) \
                       $(shell cd $(REP_DIR); find src/drivers/wifi_ath9k_drv -type f) \
					   $(LIB_MK)

MIRROR_FROM_OS_DIR  := src/lib/genode_c_api/uplink.cc

MIRROR_FROM_PC_DIR  := src/lib/pc/lx_emul \
                       $(PC_LIB_MK)

#
# DDE Linux portions (wpa_supplicant, libnl)
#

DDE_LINUX_REP_DIR  := $(GENODE_DIR)/repos/dde_linux

WS_PORT_DIR    := $(call port_dir,$(DDE_LINUX_REP_DIR)/ports/wpa_supplicant)
LIBNL_PORT_DIR := $(call port_dir,$(DDE_LINUX_REP_DIR)/ports/libnl)

DDE_LINUX_LIB_MK := \
          $(addprefix lib/mk/,libnl.inc libnl_include.mk) \
          $(foreach SPEC,x86_32 x86_64,lib/mk/spec/$(SPEC)/libnl.mk) \
          $(addprefix lib/mk/spec/x86/,wpa_driver_nl80211.mk wpa_supplicant.mk)

MIRROR_FROM_DDE_LINUX_DIR := $(DDE_LINUX_LIB_MK) \
                             lib/import/import-libnl_include.mk \
                             lib/import/import-libnl.mk \
                             include/wifi \
                             $(shell cd $(DDE_LINUX_REP_DIR); find src/lib/libnl -type f) \
                             $(shell cd $(DDE_LINUX_REP_DIR); find src/lib/wpa_driver_nl80211 -type f) \
                             $(shell cd $(DDE_LINUX_REP_DIR); find src/lib/wpa_supplicant -type f)

MIRROR_FROM_WS_PORT_DIR    := $(shell cd $(WS_PORT_DIR); find src/app/wpa_supplicant -type f)
MIRROR_FROM_LIBNL_PORT_DIR := $(shell cd $(LIBNL_PORT_DIR); find src/lib/libnl -type f)

content: $(MIRROR_FROM_REP_DIR) $(MIRROR_FROM_OS_DIR) $(MIRROR_FROM_DDE_LINUX_DIR) \
         $(MIRROR_FROM_WS_PORT_DIR) $(MIRROR_FROM_LIBNL_PORT_DIR) cleanup-wpa

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

$(MIRROR_FROM_OS_DIR): $(MIRROR_FROM_PC_DIR)
	mkdir -p $(dir $@)
	cp -r $(GENODE_DIR)/repos/os/$@ $@

$(MIRROR_FROM_PC_DIR):
	mkdir -p $(dir $@)
	cp -r $(GENODE_DIR)/repos/pc/$@ $@

$(MIRROR_FROM_DDE_LINUX_DIR):
	mkdir -p $(dir $@)
	cp -r $(GENODE_DIR)/repos/dde_linux/$@ $@

$(MIRROR_FROM_WS_PORT_DIR):
	mkdir -p $(dir $@)
	cp -r $(WS_PORT_DIR)/$@ $@

$(MIRROR_FROM_LIBNL_PORT_DIR):
	mkdir -p $(dir $@)
	cp -r $(LIBNL_PORT_DIR)/$@ $@

cleanup-wpa: $(MIRROR_FROM_WS_PORT_DIR)
	@for dir in .git doc eap_example hs20 mac80211_hwsim radius_example \
		hostapd tests wlantest wpadebug wpaspy; do \
		rm -rf src/app/wpa_supplicant/$$dir; done

content: LICENSE
LICENSE:
	( echo "Linux is subject to GNU General Public License version 2, see:"; \
	  echo "https://www.kernel.org/pub/linux/kernel/COPYING"; \
	  echo; \
	  echo "Libnl is subject to GNU LESSER GENERAL PUBLIC LICENSE Verson 2.1, see:"; \
	  echo "  src/lib/libnl/COPYING"; \
	  echo; \
	  echo "Wpa_supplicant is subject to 3-clause BSD license, see:"; \
	  echo "   src/app/wpa_supplicant/COPYING"; ) > $@

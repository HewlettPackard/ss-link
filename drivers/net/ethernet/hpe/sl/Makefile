# SPDX-License-Identifier: GPL-2.0
# Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP

include ../../../../../config.mak

MOD_NAME ?= $(SL_NAME)
SL_MOD_DIR ?= sshot

.PHONY: all default modules clean stage_install stage_uninstall

INSTALL        := install -p
TOUCH          := touch
SED            := sed
ECHO           := echo

# work around broken Debian/hms kernel build system
KNL_REL        ?= $(shell awk '/KERNELRELEASE/{print $$4}' $(KDIR)/.kernelvariables)

KNL_MOD_DIR       = $(STAGING_DIR)/lib/modules/$(KNL_REL)
KNL_TARGET        = $(shell cat '$(KDIR)/include/config/kernel.release')

MDIR = $(shell pwd)
SL_HDR_DIR = $(MDIR)/../../../../../include/linux/hpe/sl

KCPPFLAGS      += -I$(MDIR)
KCPPFLAGS      += -I$(MDIR)/media
KCPPFLAGS      += -I$(MDIR)/core
KCPPFLAGS      += -I$(MDIR)/ctrl
KCPPFLAGS      += -I$(MDIR)/log
KCPPFLAGS      += -I$(MDIR)/sysfs
KCPPFLAGS      += -I$(MDIR)/../../../../../include

ifdef PLATFORM_NIC
KCPPFLAGS      += -I$(MDIR)/../../../../../../cassini-headers/install/include
endif # PLATFORM_NIC

ifdef PLATFORM_SWITCH
KCPPFLAGS      += -I$(STAGING_DIR)/usr/include/rosetta2/csrdef
KCPPFLAGS      += -I$(STAGING_DIR)/usr/include/rosetta2/csrcntr
# the following includes are needed for the build all environment
KCPPFLAGS      += -I/usr/include/rosetta2/csrdef
KCPPFLAGS      += -I/usr/include/rosetta2/csrcntr
endif # PLATFORM_SWITCH

KCPPFLAGS      += -I$(STAGING_DIR)/usr/include
KCPPFLAGS      += -I/usr/include
KCPPFLAGS      += -Werror -Wall

KNL_BUILD_ARGS += -C $(KDIR) M=$(MDIR)
KNL_BUILD_ARGS += KCPPFLAGS="$(KCPPFLAGS)"
KNL_BUILD_ARGS += INSTALL_MOD_PATH=$(STAGING_DIR)
KNL_BUILD_ARGS += INSTALL_MOD_DIR=$(SL_MOD_DIR)
KNL_BUILD_ARGS += KBUILD_EXTRA_SYMBOLS="$(patsubst %,$(KNL_MOD_DIR)/module.symvers.%,$(KNL_EXT_SYMBOLS))"

INSTALL_LINUX_HDRS     := sl.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_ldev.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_lgrp.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_link.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_fec.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_llr.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_mac.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_media.h
INSTALL_LINUX_HDRS     += $(SL_HDR_DIR)/sl_test.h

UNINSTALL_LINUX_HDRS     := sl.h
UNINSTALL_LINUX_HDRS     += sl_ldev.h
UNINSTALL_LINUX_HDRS     += sl_lgrp.h
UNINSTALL_LINUX_HDRS     += sl_link.h
UNINSTALL_LINUX_HDRS     += sl_fec.h
UNINSTALL_LINUX_HDRS     += sl_llr.h
UNINSTALL_LINUX_HDRS     += sl_mac.h
UNINSTALL_LINUX_HDRS     += sl_media.h
UNINSTALL_LINUX_HDRS     += sl_test.h

FW_QUAD_NAME=quad
FW_QUAD_VER=3.04
FW_OCTET_NAME=octet
FW_OCTET_VER=3.08
FW_PATH=core/hw/ucode

default: all

all: stage_install

clean:
	$(MAKE) $(KNL_BUILD_ARGS) $@

modules:
	$(MAKE) $(KNL_BUILD_ARGS) $@

fw:
	$(MDIR)/../../../../../scripts/make_ucode_bin.sh $(MDIR)/$(FW_PATH)/$(FW_QUAD_NAME).h  $(MDIR)/$(FW_PATH)/sl_fw_$(FW_QUAD_NAME)_$(FW_QUAD_VER).bin
	$(MDIR)/../../../../../scripts/make_ucode_bin.sh $(MDIR)/$(FW_PATH)/$(FW_OCTET_NAME).h $(MDIR)/$(FW_PATH)/sl_fw_$(FW_OCTET_NAME)_$(FW_OCTET_VER).bin

fw_clean:
	$(RM) $(MDIR)/$(FW_PATH)/sl_fw_$(FW_QUAD_NAME)_$(FW_QUAD_VER).bin
	$(RM) $(MDIR)/$(FW_PATH)/sl_fw_$(FW_OCTET_NAME)_$(FW_OCTET_VER).bin

stage_install: modules fw
	$(INSTALL) -d $(STAGING_DIR)
	$(INSTALL) -d $(KNL_MOD_DIR)
	$(TOUCH) $(KNL_MOD_DIR)/modules.order
	$(SED) -i '/$(MOD_NAME).ko/d' $(KNL_MOD_DIR)/modules.order
	$(ECHO) $(KNL_MOD_DIR)/sshot/$(MOD_NAME).ko >> $(KNL_MOD_DIR)/modules.order
	$(TOUCH) $(STAGING_DIR)/lib/modules/$(KNL_REL)/modules.builtin
	$(MAKE) $(KNL_BUILD_ARGS) modules_install
	$(INSTALL) -m 644 Module.symvers $(KNL_MOD_DIR)/module.symvers.sl
	$(INSTALL) -d $(STAGING_DIR)/usr/include/linux/hpe/sl
	$(INSTALL) -m 644 $(INSTALL_LINUX_HDRS) $(STAGING_DIR)/usr/include/linux/hpe/sl
	$(INSTALL) -d $(STAGING_DIR)/lib/udev/rules.d
	$(INSTALL) -d $(STAGING_DIR)/usr/lib/modules-load.d
	$(INSTALL) -d $(STAGING_DIR)/usr/bin
	$(INSTALL) -m 755 ../../../../../usr/slstat $(STAGING_DIR)/usr/bin
	$(INSTALL) -d $(STAGING_DIR)/lib/firmware
	$(INSTALL) -m 755 $(MDIR)/$(FW_PATH)/sl_fw_$(FW_QUAD_NAME)_$(FW_QUAD_VER).bin   $(STAGING_DIR)/lib/firmware
	$(INSTALL) -m 755 $(MDIR)/$(FW_PATH)/sl_fw_$(FW_OCTET_NAME)_$(FW_OCTET_VER).bin $(STAGING_DIR)/lib/firmware

stage_uninstall:
	$(RM) $(patsubst %,$(STAGING_DIR)/usr/include/linux/hpe/sl/%,$(UNINSTALL_LINUX_HDRS))
	$(RM) $(KNL_MOD_DIR)/sshot/$(MOD_NAME).ko
	$(RM) $(KNL_MOD_DIR)/module.symvers.sl
	$(RM) $(STAGING_DIR)/lib/modules/$(KNL_TARGET)/sshot/$(MOD_NAME).ko
	$(RM) $(STAGING_DIR)/usr/bin/slstat
	$(RM) $(STAGING_DIR)/lib/firmware/sl_fw_$(FW_QUAD_NAME)_$(FW_QUAD_VER).bin
	$(RM) $(STAGING_DIR)/lib/firmware/sl_fw_$(FW_OCTET_NAME)_$(FW_OCTET_VER).bin
	if [ -f $(KNL_MOD_DIR)/modules.order ] ; then \
		$(SED) -i '/$(MOD_NAME).ko/d' $(KNL_MOD_DIR)/modules.order ; \
	fi

# last line

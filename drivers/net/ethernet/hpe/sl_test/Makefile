# SPDX-License-Identifier: GPL-2.0
# Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP

include ../../../../../config.mak

MOD_NAME ?= $(SL_TEST_NAME)
SL_MOD_DIR  ?= sshot

.PHONY: all default modules clean stage_install stage_uninstall

INSTALL        := install -p
TOUCH          := touch
SED            := sed
ECHO           := echo

# work around broken Debian/hms kernel build system
KNL_REL        ?= $(shell awk '/KERNELRELEASE/{print $$4}' $(KDIR)/.kernelvariables)

KNL_EXT_SYMBOLS  := sl
KNL_MOD_DIR       = $(STAGING_DIR)/lib/modules/$(KNL_REL)
KNL_TARGET        = $(shell cat '$(KDIR)/include/config/kernel.release')

MDIR = $(shell pwd)

KCPPFLAGS      += -I$(MDIR)/../sl
KCPPFLAGS      += -I$(MDIR)/../sl/media
KCPPFLAGS      += -I$(MDIR)/../sl/core
KCPPFLAGS      += -I$(MDIR)/../sl/ctrl
KCPPFLAGS      += -I$(MDIR)/../sl/log
KCPPFLAGS      += -I$(MDIR)/../../../../../include

ifdef PLATFORM_NIC
KCPPFLAGS      += -I$(MDIR)/../../../../../../cassini-headers/install/include
endif # PLATFORM_NIC

ifdef PLATFORM_SWITCH
KCPPFLAGS      += -I$(STAGING_DIR)/usr/include/rosetta2/csrdef
KCPPFLAGS      += -I/usr/include/rosetta2/csrdef
KCPPFLAGS      += -I$(STAGING_DIR)/usr/include/rosetta2/csrcntr
KCPPFLAGS      += -I/usr/include/rosetta2/csrcntr
endif # PLATFORM_SWITCH

KCPPFLAGS      += -I$(STAGING_DIR)/usr/include
KCPPFLAGS      += -I/usr/include
KCPPFLAGS      += -Werror -Wall

KNL_BUILD_ARGS += -C $(KDIR) M=$(MDIR)
KNL_BUILD_ARGS += KCPPFLAGS="$(KCPPFLAGS)"
KNL_BUILD_ARGS += INSTALL_MOD_PATH=$(STAGING_DIR)
KNL_BUILD_ARGS += INSTALL_MOD_DIR=$(SL_MOD_DIR)
KNL_BUILD_ARGS += KBUILD_EXTRA_SYMBOLS=$(MDIR)/../sl/Module.symvers

default: all

all: stage_install

modules clean:
	$(MAKE) $(KNL_BUILD_ARGS)  $@

stage_install: modules
	$(INSTALL) -d $(STAGING_DIR)
	$(INSTALL) -d $(KNL_MOD_DIR)
	$(TOUCH) $(KNL_MOD_DIR)/modules.order
	$(SED) -i '/$(MOD_NAME).ko/d' $(KNL_MOD_DIR)/modules.order
	$(ECHO) $(KNL_MOD_DIR)/$(SL_MOD_DIR)/$(MOD_NAME).ko >> $(KNL_MOD_DIR)/modules.order
	$(TOUCH) $(STAGING_DIR)/lib/modules/$(KNL_REL)/modules.builtin
	$(MAKE) $(KNL_BUILD_ARGS) modules_install
	$(INSTALL) -d $(STAGING_DIR)/usr/include/linux/hpe/sl
	$(INSTALL) -d $(STAGING_DIR)/lib/udev/rules.d
	$(INSTALL) -d $(STAGING_DIR)/usr/lib/modules-load.d
	$(INSTALL) -d $(STAGING_DIR)/usr/bin

stage_uninstall:
	$(RM) $(KNL_MOD_DIR)/$(SL_MOD_DIR)/$(MOD_NAME).ko
	$(RM) $(KNL_MOD_DIR)/module.symvers.$(MOD_NAME);
	$(RM) $(STAGING_DIR)/lib/modules/$(KNL_TARGET)/$(SL_MOD_DIR)/$(MOD_NAME).ko
	if [ -f $(KNL_MOD_DIR)/modules.order ] ; then \
		$(SED) -i '/$(MOD_NAME).ko/d' $(KNL_MOD_DIR)/modules.order ; \
	fi

# last line

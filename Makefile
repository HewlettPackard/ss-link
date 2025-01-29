# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2023,2024,2025 Hewlett Packard Enterprise Development LP
#
# Slingshot Link Driver
#

.PHONY: default setup all stage_install stage_uninstall clean install

include config.mak

SRCTOP ?= $(shell pwd)
export SRCTOP

DIRECT_TARGETS = setup install package image dist rpm

MAKECMDGOALS ?= stage_install

default: all

$(filter-out $(DIRECT_TARGETS),$(MAKECMDGOALS)) :
	@echo "Running: $@"
	$(MAKE) -C knl    $(MAKECMDGOALS)
ifdef SL_BUILD_USR
	$(MAKE) -C usr    $(MAKECMDGOALS)
endif

setup:
	./contrib/install_git_hooks.sh

all: setup
	$(MAKE) stage_install

$(INSTALL_DIR):
	mkdir -p $(INSTALL_DIR)

install: $(INSTALL_DIR)
	@echo "installing to $(INSTALL_DIR)"
	tar cf - -C $(STAGING_DIR) . |tar xf - -C $(INSTALL_DIR)

package:
	if [ -f config.mak ] ; then mv config.mak .config.mak.prev ; fi
	cp debian/changelog.template debian/changelog
	dpkg-buildpackage -uc -us -ui
	$(RM) debian/changelog
	if [ -f .config.mak.prev ] ; then mv .config.mak.prev config.mak ; fi
	install -d $(STAGING_DIR)/packages
	mv ../sl-driver*.deb $(STAGING_DIR)/packages
	mv ../sl-driver*.tar.gz $(STAGING_DIR)/packages

##########################################

PACKAGE = sl
VERSION = 0.1
DISTFILES = $(shell git ls-files 2>/dev/null || find . -type f)

all-cass:
	$(MAKE) stage_install

targz: $(DISTFILES)
	tar czf $(PACKAGE)-$(VERSION).tar.gz --transform 's/^/$(PACKAGE)-$(VERSION)\//' $(DISTFILES)

$(PACKAGE)-$(VERSION).tar.gz: targz

rpm: $(PACKAGE)-$(VERSION).tar.gz
	BUILD_METADATA='0' rpmbuild -v -ta $<

# last line

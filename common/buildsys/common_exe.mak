#
# common_exe.mak
#
#   make include file for common executable build targets
#

.PHONY: default all stage_install  stage_uninstall clean

CFLAGS  += -I$(STAGING_DIR)/usr/include
CFLAGS  += -I$(INSTALL_DIR)/usr/include
LDFLAGS += -L$(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)
LDFLAGS += -L$(INSTALL_DIR)/usr/lib/$(DISCRIMINATOR)

# there are external libs in non-standard places
# tell the linker to resolve shared libs here as well
LDFLAGS += -Wl,-rpath-link,$(INSTALL_DIR)/usr/lib/$(DISCRIMINATOR)


all : $(BINNAME)

$(BINNAME) : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(LDEXTRA)

stage_install: $(BINNAME)
	install -d $(STAGING_DIR)/usr/$(DISCRIMINATOR)/bin
	install -p $(BINNAME) $(STAGING_DIR)/usr/$(DISCRIMINATOR)/bin
ifneq ($(HDRS),)
	install -p -d $(STAGING_DIR)/usr/include
	install -p -m 644 $(HDRS) $(STAGING_DIR)/usr/include
endif
ifneq ($(ROS_HDRS),)
	install -p -d $(STAGING_DIR)/usr/include/rosetta
	install -p -m 644 $(ROS_HDRS) $(STAGING_DIR)/usr/include/rosetta
endif

stage_uninstall:
	$(RM) $(STAGING_DIR)/usr/$(DISCRIMINATOR)/bin/$(BINNAME)
ifneq ($(HDRS),)
	$(RM) $(foreach h,$(HDRS),$(STAGING_DIR)/include/$h)
endif
ifneq ($(ROS_HDRS),)
	$(RM) $(foreach h,$(HDRS),$(STAGING_DIR)/include/rosetta/$h)
endif

clean:
	$(RM) $(BINNAME)
	$(RM) $(OBJS) 
	$(RM) $(DEPS)

# last line

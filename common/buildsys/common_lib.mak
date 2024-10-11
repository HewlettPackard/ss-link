#
# common_lib.mak
#
#   common targets for building libraries
#

.PHONY: default all stage_install stage_uninstall clean
.PHONY: docs docs_stage_install docs_show

CFLAGS  += -I$(STAGING_DIR)/usr/include
CFLAGS  += -I$(INSTALL_DIR)/usr/include
LDFLAGS += -L$(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)
LDFLAGS += -L$(INSTALL_DIR)/usr/lib/$(DISCRIMINATOR)

# there are external libs in non-standard places
# tell the linker to resolve shared libs here as well
LDFLAGS += -Wl,-rpath-link,$(INSTALL_DIR)/usr/lib/$(DISCRIMINATOR)

all : src/$(SOLIB2) src/lib$(LIBNAME).a

src/$(SOLIB2) : $(OBJS)
	$(CC) -shared -fPIC -Wl,-soname,$(SOLIB1) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) -pthread -lc

src/lib$(LIBNAME).a : $(OBJS)
	$(AR) rD $@ $(OBJS)

stage_install: src/$(SOLIB2) src/lib$(LIBNAME).a
ifneq ($(HDRS),)
	install -p -d $(STAGING_DIR)/usr/include
	install -p -m 644 $(HDRS) $(STAGING_DIR)/usr/include
endif
ifneq ($(ROS_HDRS),)
	install -p -d $(STAGING_DIR)/usr/include/rosetta
	install -p -m 644 $(ROS_HDRS) $(STAGING_DIR)/usr/include/rosetta
endif
ifneq ($(SHARE_FILES),)
	install -d $(STAGING_DIR)/usr/share/rosetta
	install -p -m 644 $(patsubst %,share/%,$(SHARE_FILES)) $(STAGING_DIR)/usr/share/rosetta
endif
	install -d $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)
	install -p -m 644 src/$(SOLIB2) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)
	install -p -m 644 src/lib$(LIBNAME).a $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)
	ln -f -s ./$(SOLIB2) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)/$(SOLIB0)
	ln -f -s ./$(SOLIB2) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)/$(SOLIB1)

stage_uninstall:
	$(RM) $(foreach h,$(HDRS),$(STAGING_DIR)/include/$h)
	$(RM) $(foreach h,$(HDRS),$(STAGING_DIR)/include/rosetta/$h)
	$(RM) $(foreach f,$(SHARE_FILES),$(STAGING_DIR)/usr/share/rosetta/$f)
	$(RM) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)/$(SOLIB0)
	$(RM) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)/$(SOLIB1)
	$(RM) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)/$(SOLIB2)
	$(RM) $(STAGING_DIR)/usr/lib/$(DISCRIMINATOR)/lib$(LIBNAME).a
	$(RM) -r $(STAGING_DIR)/usr/doc/lib$(LIBNAME)

clean:
	$(RM) $(wildcard src/$(SOLIB0).*)
	$(RM) src/lib$(LIBNAME).a
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) -r doc/generated

doc/generated/lib$(LIBNAME)/html/index.html:  $(DOXYGEN_MAIN) $(DOC_FILES)
	install -d doc/generated
	doxygen $(DOXYGEN_MAIN) 

docs:  doc/generated/lib$(LIBNAME)/html/index.html

docs_stage_install: doc/generated/lib$(LIBNAME)/html/index.html
	install -d  $(STAGING_DIR)/usr/share/doc
	cp -rp  doc/generated/lib$(LIBNAME)  $(STAGING_DIR)/usr/share/doc

docs_stage_uninstall:
	$(RM) -r $(STAGING_DIR)/usr/share/doc/lib$(LIBNAME)

docs_show:
	firefox doc/generated/lib$(LIBNAME)/html/index.html

# last line

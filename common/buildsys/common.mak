#
# common.mak
#
# rules to compile an object file and create dependency file
#

DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(USR_BUILD_ARGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $*.Td $*.dep && touch $@

%.o : %.dep
%.o : %.c 
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

# last line
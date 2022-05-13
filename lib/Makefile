#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

SUBDIRS = logs
OBJS    = iommap.o stack.o list.o array.o

lib.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS) logs/logs.o

include $(TOPDIR)/Rules.makefile

### Dependencies ###

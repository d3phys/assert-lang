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
array.o: array.cpp /home/d3phys/Code/assert-language/include/array.h
iommap.o: iommap.cpp /home/d3phys/Code/assert-language/include/iommap.h
list.o: list.cpp /home/d3phys/Code/assert-language/include/list.h
listdev.o: listdev.cpp /home/d3phys/Code/assert-language/include/list.h
stack.o: stack.cpp /home/d3phys/Code/assert-language/include/stack.h

#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

OBJS = transpile.o

trans.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS)

include $(TOPDIR)/Rules.makefile

### Dependencies ###
main.o: main.cpp /home/d3phys/Code/assert-language/include/logs.h \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/iommap.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h
transpile.o: transpile.cpp \
 /home/d3phys/Code/assert-language/include/iommap.h \
 /home/d3phys/Code/assert-language/include/logs.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/ast/keyword.h \
 /home/d3phys/Code/assert-language/include/ast/../../AST \
 /home/d3phys/Code/assert-language/include/frontend/keyword.h \
 /home/d3phys/Code/assert-language/include/../KEYWORDS

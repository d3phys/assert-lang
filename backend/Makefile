#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

OBJS = compiler.o scope_table.o

backend.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS)

include $(TOPDIR)/Rules.makefile

### Dependencies ###
compiler.o: compiler.cpp /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/array.h \
 /home/d3phys/Code/language/include/iommap.h \
 /home/d3phys/Code/language/include/stack.h \
 /home/d3phys/Code/language/include/ast/tree.h \
 /home/d3phys/Code/language/include/ast/keyword.h \
 /home/d3phys/Code/language/include/ast/../../AST \
 /home/d3phys/Code/language/include/backend/scope_table.h \
 /home/d3phys/Code/language/include/backend/backend.h
main.o: main.cpp /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/array.h \
 /home/d3phys/Code/language/include/iommap.h \
 /home/d3phys/Code/language/include/stack.h \
 /home/d3phys/Code/language/include/ast/tree.h \
 /home/d3phys/Code/language/include/ast/keyword.h \
 /home/d3phys/Code/language/include/ast/../../AST \
 /home/d3phys/Code/language/include/backend/scope_table.h \
 /home/d3phys/Code/language/include/backend/backend.h
scope_table.o: scope_table.cpp /home/d3phys/Code/language/include/array.h \
 /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/ast/tree.h \
 /home/d3phys/Code/language/include/backend/scope_table.h

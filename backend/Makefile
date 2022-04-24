#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

OBJS = compiler.o elf64.o  #compiler.o scope_table.o  iencode.o

backend.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS)

include $(TOPDIR)/Rules.makefile

### Dependencies ###
compiler.o: compiler.cpp \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/iommap.h \
 /home/d3phys/Code/assert-language/include/stack.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/ast/keyword.h \
 /home/d3phys/Code/assert-language/include/ast/../../AST \
 /home/d3phys/Code/assert-language/include/backend/scope_table.h \
 /home/d3phys/Code/assert-language/include/backend/backend.h \
 /home/d3phys/Code/assert-language/include/backend/iencode.h
elf64.o: elf64.cpp /home/d3phys/Code/assert-language/include/iommap.h \
 /home/d3phys/Code/assert-language/include/backend/elf64.h \
 /home/d3phys/Code/assert-language/include/backend/backend.h \
 /home/d3phys/Code/assert-language/include/stack.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/backend/iencode.h ../STDLIB
main.o: main.cpp /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/iommap.h \
 /home/d3phys/Code/assert-language/include/stack.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/ast/keyword.h \
 /home/d3phys/Code/assert-language/include/ast/../../AST \
 /home/d3phys/Code/assert-language/include/backend/scope_table.h \
 /home/d3phys/Code/assert-language/include/backend/backend.h \
 /home/d3phys/Code/assert-language/include/backend/iencode.h
scope_table.o: scope_table.cpp \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/backend/scope_table.h

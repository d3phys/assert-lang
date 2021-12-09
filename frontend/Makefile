#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

OBJS = lexer.o dump_tree.o ident.o tree.o recursive_descent.o

frontend.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS)

include $(TOPDIR)/Rules.makefile

### Dependencies ###
dump_tree.o: dump_tree.cpp /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/frontend/tree.h \
 /home/d3phys/Code/language/include/frontend/grammar.h
ident.o: ident.cpp /home/d3phys/Code/language/include/frontend/grammar.h
lexer.o: lexer.cpp /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/list.h \
 /home/d3phys/Code/language/include/array.h \
 /home/d3phys/Code/language/include/frontend/token.h \
 /home/d3phys/Code/language/include/frontend/tree.h \
 /home/d3phys/Code/language/include/frontend/grammar.h
list.o: list.cpp /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/list.h
recursive_descent.o: recursive_descent.cpp \
 /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/frontend/tree.h \
 /home/d3phys/Code/language/include/frontend/token.h \
 /home/d3phys/Code/language/include/list.h
tree.o: tree.cpp /home/d3phys/Code/language/include/logs.h \
 /home/d3phys/Code/language/include/frontend/tree.h

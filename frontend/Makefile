#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

OBJS = lexer.o ident.o recursive_descent.o

frontend.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS)

include $(TOPDIR)/Rules.makefile

### Dependencies ###
ident.o: ident.cpp \
 /home/d3phys/Code/assert-language/include/frontend/keyword.h \
 /home/d3phys/Code/assert-language/include/../KEYWORDS ../KEYWORDS
lexer.o: lexer.cpp /home/d3phys/Code/assert-language/include/logs.h \
 /home/d3phys/Code/assert-language/include/list.h \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/frontend/token.h \
 /home/d3phys/Code/assert-language/include/frontend/keyword.h \
 /home/d3phys/Code/assert-language/include/../KEYWORDS ../KEYWORDS
main.o: main.cpp /home/d3phys/Code/assert-language/include/logs.h \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/iommap.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/frontend/token.h \
 /home/d3phys/Code/assert-language/include/frontend/compile.h
recursive_descent.o: recursive_descent.cpp \
 /home/d3phys/Code/assert-language/include/logs.h \
 /home/d3phys/Code/assert-language/include/ast/tree.h \
 /home/d3phys/Code/assert-language/include/array.h \
 /home/d3phys/Code/assert-language/include/ast/keyword.h \
 /home/d3phys/Code/assert-language/include/ast/../../AST \
 /home/d3phys/Code/assert-language/include/frontend/keyword.h \
 /home/d3phys/Code/assert-language/include/../KEYWORDS \
 /home/d3phys/Code/assert-language/include/frontend/token.h

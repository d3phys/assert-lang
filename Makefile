#
# Main akinator makefile
#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
#
# 2021, d3phys
#

#
# Awesome flags collection
# Copyright (C) 2021, 2022 ded32, the TXLib creator
#
TXXFLAGS = -g --static-pie -std=c++14 -fmax-errors=100 -Wall -Wextra  	   \
	   -Weffc++ -Waggressive-loop-optimizations -Wc++0x-compat 	   \
	   -Wc++11-compat -Wc++14-compat -Wcast-align -Wcast-qual 	   \
	   -Wchar-subscripts -Wconditionally-supported -Wconversion        \
	   -Wctor-dtor-privacy -Wempty-body -Wfloat-equal 		   \
	   -Wformat-nonliteral -Wformat-security -Wformat-signedness       \
	   -Wformat=2 -Winline -Wlarger-than=8192 -Wlogical-op 	           \
	   -Wmissing-declarations -Wnon-virtual-dtor -Wopenmp-simd 	   \
	   -Woverloaded-virtual -Wpointer-arith -Wredundant-decls          \
	   -Wshadow -Wsign-conversion -Wsign-promo -Wstack-usage=8192      \
	   -Wstrict-null-sentinel -Wstrict-overflow=2 			   \
	   -Wsuggest-attribute=noreturn -Wsuggest-final-methods 	   \
	   -Wsuggest-final-types -Wsuggest-override -Wswitch-default 	   \
	   -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused   \
	   -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix 	   \
	   -Wno-missing-field-initializers -Wno-narrowing 	           \
	   -Wno-old-style-cast -Wno-varargs -fcheck-new 		   \
	   -fsized-deallocation -fstack-check -fstack-protector            \
	   -fstrict-overflow -flto-odr-type-merging 	   		   \
	   -fno-omit-frame-pointer                                         \
	   -fPIE                                                           \
	   -fsanitize=address 	                                           \
	   -fsanitize=alignment                                            \
	   -fsanitize=bool                                                 \
 	   -fsanitize=bounds                                               \
	   -fsanitize=enum                                                 \
	   -fsanitize=float-cast-overflow 	                           \
	   -fsanitize=float-divide-by-zero 			           \
	   -fsanitize=integer-divide-by-zero                               \
	   -fsanitize=leak 	                                           \
	   -fsanitize=nonnull-attribute                                    \
	   -fsanitize=null 	                                           \
	   -fsanitize=object-size                                          \
	   -fsanitize=return 		                                   \
	   -fsanitize=returns-nonnull-attribute                            \
	   -fsanitize=shift                                                \
	   -fsanitize=signed-integer-overflow                              \
	   -fsanitize=undefined                                            \
	   -fsanitize=unreachable                                          \
	   -fsanitize=vla-bound                                            \
	   -fsanitize=vptr                                                 \
	   -lm -pie

SUBDIRS = lib frontend ast backend/llvm backend/legacy trans

CXX = g++
CPP = $(CXX) -E
TOPDIR	:= $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)


CXXFLAGS = $(HFLAGS) $(LOGSFLAGS) $(TXXFLAGS)
LOGSFLAGS = -D 'LOGS_FILE="logs.html"' -D LOGS_COLORS -D LOGS_DEBUG

# Header files

HFLAGS = $(addprefix -I, $(HPATH))
HPATH  = $(TOPDIR)/include 	\
	 $(TOPDIR)/lib/logs

ASSEMBLY_PATH = $(TOPDIR)/assembly/include

make: dep front back trans back-llvm
	nasm -f elf64 -o asslib.o asslib.s
	@printf "\n\n\n\n\n\n"
	@echo "Assert language is compiled now!"
	@echo "Read: https://d3phys.github.io/assert-book/"

commit: clean rmdep
	@echo "Ready for commit"

test: front back-llvm
	./tr examples/fucktorial2 fuck.tree
	./cum-llvm fuck.tree fuck.ir > test.ir
	cat test.ir
	llc test.ir -filetype=obj -o test.o
	gcc test.o

quadr: back front
	./tr examples/quadratic-integer test_tree
	./cum test_tree asm
	ld -o test asm asslib.o /lib64/libc.so.6 -I/lib64/ld-linux-x86-64.so.2

fuck: back front
	./tr examples/fucktorial test_tree
	./cum test_tree asm
	ld -o test asm asslib.o /lib64/libc.so.6 -I/lib64/ld-linux-x86-64.so.2

test-elf: subdirs backend/legacy/test/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o test-elf lib/lib.o backend/legacy/backend.o \
			      frontend/frontend.o ast/ast.o backend/legacy/test/main.o
	./test-elf

front: subdirs frontend/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o tr lib/lib.o \
			      frontend/frontend.o ast/ast.o frontend/main.o

back: subdirs backend/legacy/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o cum lib/lib.o backend/legacy/backend.o \
			      frontend/frontend.o ast/ast.o backend/legacy/main.o

back-llvm: CXXFLAGS+=$(shell llvm-config --cppflags)
back-llvm: LDFLAGS+=$(shell llvm-config --ldflags)
back-llvm: LIBS+=$(shell llvm-config --libs)
back-llvm: subdirs backend/llvm/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o cum-llvm lib/lib.o backend/llvm/backend.o \
			      frontend/frontend.o ast/ast.o backend/llvm/main.o

trans: subdirs trans/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o rev lib/lib.o frontend/frontend.o\
			      trans/trans.o ast/ast.o trans/main.o

mur: subdirs
	$(CXX) $(CXXFLAGS) -o mur utils/mur.o lib/lib.o

touch:
	@find $(HPATH) -print -exec touch {} \;

.EXPORT_ALL_VARIABLES: CXX CXXFLAGS CPP

include $(TOPDIR)/Rules.makefile

### Dependencies ###

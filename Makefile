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
	   -Woverloaded-virtual -Wpacked -Wpointer-arith -Wredundant-decls \
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

SUBDIRS = lib frontend ast backend trans

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

make: subdirs
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o build lib/lib.o frontend/frontend.o \
			      ast/ast.o backend/backend.o trans/trans.o
	./build	

tback: subdirs
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o cum lib/lib.o backend/backend.o \
			      frontend/frontend.o ast/ast.o
	./cum

front: subdirs frontend/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o tr lib/lib.o \
			      frontend/frontend.o ast/ast.o frontend/main.o

back: subdirs backend/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o cum lib/lib.o backend/backend.o \
			      frontend/frontend.o ast/ast.o backend/main.o

trans: subdirs trans/main.o
	$(OBJS)
	$(CXX) $(CXXFLAGS) -o rev lib/lib.o frontend/frontend.o\
			      trans/trans.o ast/ast.o trans/main.o

mur: subdirs
	$(CXX) $(CXXFLAGS) -o mur utils/mur.o lib/lib.o

compile: make 
	rm tree compiled ex
	./front code tree
	./build
	assembly/ass -i compiled -o ex -s
	assembly/exe ex

test: make 
	$(CXX) $(CXXFLAGS) -o tst test/test_logs.o core/core.o lib/lib.o ast/ast.o
	./tst

touch:
	@find $(HPATH) -print -exec touch {} \;

.EXPORT_ALL_VARIABLES: CXX CXXFLAGS CPP

include $(TOPDIR)/Rules.makefile

### Dependencies ###

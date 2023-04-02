#
# Important! Dependencies are done automatically by 'make dep', which also
# removes any old dependencies. Do not modify it...
# 2021, d3phys
#

OBJS = ir_gen.o

LLVMCONFIG="llvm-config"
LLVM_CXXFLAGS="${$LLVMCONFIG --cppflags}"
LLVM_LDFLAGS="${$LLVMCONFIG --ldflags}"
LLVM_LIBS="${$LLVMCONFIG --libs}"

backend.o: $(OBJS) subdirs
	$(LD) -r -o $@ $(OBJS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -I$(HPATH) -I$(ASSEMBLY_PATH) $< -o $@

include $(TOPDIR)/Rules.makefile

### Dependencies ###

#
# This file contains rules which are shared between multiple Makefiles.
# 2021, d3phys
#

#
# Special variables which should not be exported
#
unexport SUBDIRS
unexport OBJS

clean:
	@rm -f *.o 
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) clean); done
	
subdirs:
	@for i in $(SUBDIRS); do (cd $$i && echo $$i && $(MAKE)) || exit; done

#
# Dependencies 
#
dep:
	@sed '/\#\#\# Dependencies \#\#\#/q' < Makefile > temp_make
	@if [[ "$(wildcard *.cpp)" != "" ]]; then $(CPP) -MM *.cpp $(addprefix -I, $(HPATH)) >> temp_make; fi
	@cp temp_make Makefile
	@rm temp_make
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) dep) || exit; done

rmdep:
	@sed '/\#\#\# Dependencies \#\#\#/q' < Makefile > temp_make
	@cp temp_make Makefile
	@rm temp_make
	@for i in $(SUBDIRS); do (cd $$i && $(MAKE) rmdep) || exit; done	
#
# Common rules
#
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $(LIBS) -I$(HPATH) -I$(ASSEMBLY_PATH) $< -o $@


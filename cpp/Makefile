#Makefile for gallery c++ programs.
#Note, being all-incllusive here: you can cut out libraries/includes you do not need
#you can also change the flags if you want too (Werror, pedantic, etc.)

CPPFLAGS=-I $(BOOST_INC) \
         -I $(CANVAS_INC) \
         -I $(CETLIB_INC) \
         -I $(FHICLCPP_INC) \
         -I $(GALLERY_INC) \
         -I $(LARCOREOBJ_INC) \
         -I $(LARDATAOBJ_INC) \
         -I $(NUSIMDATA_INC) \
         -I $(ROOT_INC)

CXXFLAGS=-std=c++14 -Wall -Werror -pedantic
CXX=g++
LDFLAGS=$$(root-config --libs) \
        -L $(CANVAS_LIB) -l canvas_Utilities -l canvas_Persistency_Common -l canvas_Persistency_Provenance \
        -L $(CETLIB_LIB) -l cetlib \
        -L $(GALLERY_LIB) -l gallery \
        -L $(NUSIMDATA_LIB) -l nusimdata_SimulationBase \
        -L $(LARCOREOBJ_LIB) -l larcoreobj_SummaryData \
        -L $(LARDATAOBJ_LIB) -l lardataobj_Simulation -l lardataobj_RecoBase -l lardataobj_MCBase -l lardataobj_RawData -l lardataobj_OpticalDetectorData -l lardataobj_AnalysisBase

demo_ReadEvent: demo_ReadEvent.cc
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

demo_ReadOpFlashes: demo_ReadOpFlashes.cc hist_utilities.h
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

demo_ReadOpFlashes_MakeTree: demo_ReadOpFlashes_MakeTree.cc hist_utilities.h
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

demo_ReadClusters_MakeTree: demo_ReadClusters_MakeTree.cc hist_utilities.h
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

SimpleOpFlashAna.o: SimpleOpFlashAna.cxx SimpleOpFlashAna.hh
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -c SimpleOpFlashAna.cxx

demo_SimpleOpFlashAna: demo_SimpleOpFlashAna.cc SimpleOpFlashAna.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) SimpleOpFlashAna.o -o $@ $<

all: demo_ReadEvent demo_ReadOpFlashes demo_ReadOpFlashes_MakeTree demo_ReadClusters_MakeTree

clean:
	rm *.o demo_ReadEvent demo_ReadOpFlashes demo_ReadOpFlashes_MakeTree demo_SimpleOpFlashAna

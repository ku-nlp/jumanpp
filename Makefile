
CXX := g++
#CXXFLAGS := -O3 -Wall # -g
CXXFLAGS := --std=c++1y -g3 -O3 -m64 -Wall -Wl,-rpath /share/usr-x86_64/lib64/ # -pg
CXXFLAGS_G := --std=c++1y -g3 -fno-inline -g -m64 -Wall  -Wl,-rpath /share/usr-x86_64/lib64/ # -pg

#ifdef LD_RUN_PATH
#CXXFLAGS := $(CXXFLAGS) -Wl,-rpath $(LD_RUN_PATH)
#else
#CXXFLAGS := $(CXXFLAGS)
#endif

OBJECTS_TEST := kkn_test.ot wvector.ot wvector_test.ot morph.ot dic.ot tagger.ot pos.ot sentence.ot feature.ot node.ot tools.ot charlattice.ot scw.ot rnnlm/rnnlmlib.ot
OBJECTS := morph.o dic.o tagger.o pos.o sentence.o feature.o node.o tools.o charlattice.o u8string.o scw.o cdb_juman.o cdb/libcdb.a rnnlm/rnnlmlib.o
HEADERS := wvector.h common.h dic.h tagger.h pos.h sentence.h feature.h node.h parameter.h u8string.h scw.h cdb_juman.h rnnlm/rnnlmlib.h
LIBS := -lprofiler

MKDARTS_OBJECTS := mkdarts.o pos.o
MKDARTS_HEADERS := tagger.h pos.h


%.o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.og: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS_G) -c -o $@ $<

%.og: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS_G) -c -o $@ $<

%.ot: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $< -D KKN_UNIT_TEST 

%.o: %.c $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.og: %.c $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
%.ot: %.c $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $< -D KKN_UNIT_TEST 


all: kkn mkdarts 

cdb/libcdb.a:
	cd cdb; make

kkn_test: $(OBJECTS_TEST) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -lboost_unit_test_framework -o $@ $(OBJECTS_TEST) -D KKN_UNIT_TEST
	#$(CXX) $(CXXFLAGS) /usr/local/lib/libboost_unit_test_framework.dylib  -o $@ $(OBJECTS_TEST) -D KKN_UNIT_TEST

kkn: $(OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -o $@ $(OBJECTS)

kkn_g: $(OBJECTS:%.o=%.og) $(HEADERS)
	$(CXX) $(CXXFLAGS_G) $(LD_FLAGS) $(LIBS) -o $@ $(OBJECTS:%.o=%.og)

mkdarts: $(MKDARTS_OBJECTS) $(MKDARTS_HEADERS)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -o $@ $(MKDARTS_OBJECTS)

clean:
	rm -f $(OBJECTS_TEST) $(OBJECTS) $(MKDARTS_OBJECTS) kkn kkn_g mkdarts kkn_test

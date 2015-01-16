CXX := g++
#CXXFLAGS := -O3 -Wall # -g
CXXFLAGS := --std=c++1y -O3 -g -m64 -w #-Wl,-rpath /share/usr-x86_64/lib64 # -pg

OBJECTS_TEST := kkn_test.ot wvector.ot wvector_test.ot morph.ot dic.ot tagger.ot pos.ot sentence.ot feature.ot node.ot tools.ot charlattice.ot
OBJECTS := morph.o dic.o tagger.o pos.o sentence.o feature.o node.o tools.o charlattice.o u8string.o
HEADERS := wvector.h common.h dic.h tagger.h pos.h sentence.h feature.h node.h parameter.h u8string.h

MKDARTS_OBJECTS := mkdarts.o pos.o
MKDARTS_HEADERS := tagger.h pos.h

%.o: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.ot: %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $< -D KKN_UNIT_TEST 

all: kkn mkdarts 

kkn_test: $(OBJECTS_TEST) $(HEADERS)
	$(CXX) $(CXXFLAGS) -lboost_unit_test_framework -o $@ $(OBJECTS_TEST) -D KKN_UNIT_TEST
	#$(CXX) $(CXXFLAGS) /usr/local/lib/libboost_unit_test_framework.dylib  -o $@ $(OBJECTS_TEST) -D KKN_UNIT_TEST

kkn: $(OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

kkn_g: $(OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

mkdarts: $(MKDARTS_OBJECTS) $(MKDARTS_HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(MKDARTS_OBJECTS)

clean:
	rm -f $(OBJECTS_TEST) $(OBJECTS) $(MKDARTS_OBJECTS) kkn kkn_g mkdarts kkn_test


CXX := g++
CXXFLAGS := --std=c++1y -Ofast -m64 -Wno-c++11-narrowing -lm -Wall -funroll-loops -ffast-math #-DUSE_SRILM -DINSTANTIATE_TEMPLATES -lpthread -Isrilm/include  # -funroll-loops -ffast-math
CXXFLAGS_G := --std=c++1y -g -Og -m64 -Wno-c++11-narrowing -ltcmalloc -lm -fno-omit-frame-pointer -Wall -funroll-loops -ffast-math

SRILM_PATH := /home/morita/archive/srilm

#OBJECTS_TEST := kkn_test.ot wvector.ot wvector_test.ot morph.ot dic.ot tagger.ot pos.ot sentence.ot feature.ot node.ot tools.ot charlattice.ot scw.ot rnnlm/rnnlmlib.ot
SRILM_OBJS := \
        $(SRILM_PATH)/lib/i686-m64/libflm.a \
        $(SRILM_PATH)/lib/i686-m64/liboolm.a \
        $(SRILM_PATH)/lib/i686-m64/liblattice.a \
        $(SRILM_PATH)/lib/i686-m64/libdstruct.a \
        $(SRILM_PATH)/lib/i686-m64/libmisc.a \
        $(SRILM_PATH)/lib/i686-m64/libz.a \

OBJECTS := morph.o dic.o tagger.o pos.o sentence.o feature.o node.o tools.o charlattice.o u8string.o scw.o cdb_juman.o cdb/libcdb.a rnnlm/rnnlmlib.o #srilm/NgramLM.o srilm/Vocab.o
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
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -lboost_unit_test_framework -o $@ $(OBJECTS_TEST) $(SRILM_OBJS) -D KKN_UNIT_TEST

kkn: $(OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -lboost_serialization  -o $@ $(OBJECTS) $(SRILM_OBJS)

kkn_g: $(OBJECTS:%.o=%.og) $(HEADERS)
	$(CXX) $(CXXFLAGS_G) $(LD_FLAGS) $(LIBS) -lboost_serialization  -o $@ $(OBJECTS:%.o=%.og) $(SRILM_OBJS)

mkdarts: $(MKDARTS_OBJECTS) $(MKDARTS_HEADERS)
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) -lboost_serialization  -o $@ $(MKDARTS_OBJECTS)

clean:
	rm -f $(OBJECTS_TEST) $(OBJECTS) $(MKDARTS_OBJECTS) kkn kkn_g mkdarts kkn_test

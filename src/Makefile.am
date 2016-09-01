AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS = --std=c++11 -Ofast -m64 -Wall -funroll-loops -ffast-math

bin_PROGRAMS=jumanpp mkdarts_jumanpp
# noinst_PROGRAMS=mkdarts

jumanpp_SOURCES = charlattice.cc dic.cc feature.cc morph.cc node.cc pos.cc scw.cc sentence.cc tagger.cc tools.cc u8string.cc cdb_juman.cc rnnlm/rnnlmlib_dynamic.cpp rnnlm/rnnlmlib_static.cpp cdb_juman.h charlattice.h cmdline.h common.h darts.h dic.h feature.h node.h parameter.h pos.h scw.h sentence.h tagger.h u8string.h rnnlm/rnnlmlib_dynamic.h rnnlm/rnnlmlib_static.h rnnlm/rnnlmlib.h mmap.h feature_vector.cc feature_vector.h

mkdarts_jumanpp_SOURCES = mkdarts.cc pos.cc pos.h tagger.h 



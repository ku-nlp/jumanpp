AUTOMAKE_OPTIONS = subdir-objects

GIT_VER=$(shell git log -1 --date=local --format="%ad %h")
AM_CXXFLAGS = --std=c++1y -Ofast -m64 -Wall -funroll-loops -ffast-math -DGITVER="\"$(GIT_VER)\""

bin_PROGRAMS=jumanpp 
noinst_PROGRAMS=mkdarts

jumanpp_SOURCES = charlattice.cc dic.cc feature.cc morph.cc node.cc pos.cc scw.cc sentence.cc tagger.cc tools.cc u8string.cc cdb_juman.cc rnnlm/rnnlmlib_dynamic.cpp rnnlm/rnnlmlib_static.cpp cdb_juman.h charlattice.h cmdline.h common.h darts.h dic.h feature.h node.h parameter.h pos.h scw.h sentence.h tagger.h u8string.h 

mkdarts_SOURCES = mkdarts.cc pos.cc pos.h tagger.h



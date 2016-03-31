
AUTOMAKE_OPTIONS = subdir-objects

GIT_VER=$(shell git log -1 --date=local --format="%ad %h")
AM_CXXFLAGS = --std=c++1y -Ofast -m64 -Wall -funroll-loops -ffast-math -DGITVER="\"$(GIT_VER)\""

bin_PROGRAMS=jumanpp mkdarts 
jumanpp_SOURCES = src/charlattice.cc src/dic.cc src/feature.cc src/morph.cc src/node.cc src/pos.cc src/scw.cc src/sentence.cc src/tagger.cc src/tools.cc src/u8string.cc src/cdb_juman.cc src/rnnlm/rnnlmlib_dynamic.cpp src/rnnlm/rnnlmlib_static.cpp src/cdb_juman.h src/charlattice.h src/cmdline.h src/common.h src/darts.h src/dic.h src/feature.h src/node.h src/parameter.h src/pos.h src/scw.h src/sentence.h src/tagger.h src/u8string.h 

mkdarts_SOURCES = src/mkdarts.cc src/pos.cc src/pos.h src/tagger.h


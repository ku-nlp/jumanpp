#!/bin/sh

# This script builds Juman++ with Profile-Guided Optimizations & Link-Time optimizations

FULL_NAME=$(readlink -f "$0")

SCRIPT_DIR=$(dirname "$FULL_NAME")

JPP_SRC=$(dirname "$SCRIPT_DIR")

JPP_ARCH=x86-64-v2

JPP_DATA=

for i in "$@"; do
  case $i in
    --arch=*)
      JPP_ARCH="${i#*=}"
      shift
      ;;
    --data=*)
      JPP_DATA="${i#*=}"
      shift
      ;;
    --model=*)
      JPP_MODEL="${i#*=}"
      shift
      ;;
    -DCMAKE_CXX_FLAGS=*)
      JPP_CXX_FLAGS="${i#*=}"
      shift
      ;;
    *)
      shift
      ;;
  esac
done

if [ ! -f "$JPP_DATA" ]; then
  echo "Data for PGO collection was not passed. Pass a file with Japanese text."
  exit 1
fi

if [ ! -f "$JPP_MODEL" ]; then
  echo "Juman++ model was not passed"
  exit 1
fi


BLD_DIR_NAME=bld-${JPP_ARCH}

JPP_BLD="$JPP_SRC/$BLD_DIR_NAME"

mkdir -p "$JPP_BLD"

#delete PGO data
rm -rf  "/tmp/jpp-bld/prof-${JPP_ARCH}"

# GCC 10.1+ supports -fprofile-prefix-path=$JPP_SRC

# Configure collection of profile data
cmake -S "$JPP_SRC" -B "$JPP_BLD" \
    -DJPP_ENABLE_TESTS=FALSE \
    -DJPP_MAX_DIC_FIELDS=10 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=$JPP_ARCH -fprofile-dir=/tmp/jpp-bld/prof-${JPP_ARCH} -fprofile-generate -flto -fuse-linker-plugin ${JPP_CXX_FLAGS}" \
    -DCMAKE_CXX_FLAGS_RELEASE='-Ofast -fgcse-sm -fgcse-las -fipa-pta -ffast-math -ffinite-math-only -fno-signed-zeros -fomit-frame-pointer -DNDEBUG'

# Compile profile-collecting binary
nice -n19 cmake --build "$JPP_BLD" --parallel -- jumanpp_v2

# Collect profiling data
echo "Collecting PGO data for $JPP_ARCH"
nice "$JPP_BLD/src/jumandic/jumanpp_v2" "--model=${JPP_MODEL}" "$JPP_DATA" --format=juman -o /dev/null
nice "$JPP_BLD/src/jumandic/jumanpp_v2" "--model=${JPP_MODEL}" "$JPP_DATA" --format=lattice -o /dev/null

# Configure PGO compilation
cmake -S "$JPP_SRC" -B "$JPP_BLD" \
    -DJPP_ENABLE_TESTS=FALSE \
    -DJPP_ENABLE_TESTS=FALSE \
    -DJPP_MAX_DIC_FIELDS=10 \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-march=$JPP_ARCH -fprofile-dir=/tmp/jpp-bld/prof-${JPP_ARCH} -fprofile-use -flto -fuse-linker-plugin ${JPP_CXX_FLAGS}" \
    -DCMAKE_CXX_FLAGS_RELEASE='-Ofast -fgcse-sm -fgcse-las -fipa-pta -ffast-math -ffinite-math-only -fno-signed-zeros -fomit-frame-pointer -DNDEBUG'

# Build final binary
nice -n19 cmake --build "$JPP_BLD" --parallel -- jumanpp_v2


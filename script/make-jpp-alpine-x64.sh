#!/bin/sh

set -x

# https://unix.stackexchange.com/questions/631217/how-do-i-check-if-my-cpu-supports-x86-64-v2
awk -f - <<EOF
BEGIN {
    while (!/flags/) if (getline < "/proc/cpuinfo" != 1) exit 1
    if (/lm/&&/cmov/&&/cx8/&&/fpu/&&/fxsr/&&/mmx/&&/syscall/&&/sse2/) level = 1
    if (level == 1 && /cx16/&&/lahf/&&/popcnt/&&/sse4_1/&&/sse4_2/&&/ssse3/) level = 2
    if (level == 2 && /avx/&&/avx2/&&/bmi1/&&/bmi2/&&/f16c/&&/fma/&&/abm/&&/movbe/&&/xsave/) level = 3
    if (level == 3 && /avx512f/&&/avx512bw/&&/avx512cd/&&/avx512dq/&&/avx512vl/) level = 4
    if (level > 0) { exit level + 1 }
    exit 1
}
EOF

LEVEL=$?

FULL_NAME=$(readlink -f "$0")

SCRIPT_DIR=$(dirname "$FULL_NAME")

JPP_SRC=$(dirname "$SCRIPT_DIR")

MODEL_FILE="$JPP_SRC/model/jumandic.jppmdl"

TEXT_DATA="$JPP_SRC/model/pgo-train.txt"

if [[ ! -f "$MODEL_FILE" ]]; then
  echo "Model file $MODEL_FILE was not found"
  exit 1
fi

if [[ ! -f "$TEXT_DATA" ]]; then
  # download file
  curl -L https://github.com/ku-nlp/KWDLC/releases/download/release_1_0/leads.org.txt.gz | gzip -dc > "$TEXT_DATA"
fi

# build and install default version
$SHELL "$SCRIPT_DIR/build-pgo.sh" "--arch=x86-64" "--model=$MODEL_FILE" "--data=$TEXT_DATA" -DCMAKE_CXX_FLAGS=-U_FORTIFY_SOURCE

# this will install default config/model to /usr/local
cmake --install "$JPP_SRC/bld-x86-64"

RESDIR="$JPP_SRC/bld-docker-dist/usr/local"

install -m 0755 -D /usr/local/bin/jumanpp "$JPP_SRC/bld-docker-dist/usr/local/bin/jumanpp-default"
install -m 0644 -D /usr/local/libexec/jumanpp/jumandic.jppmdl "$RESDIR/libexec/jumanpp/jumandic.jppmdl"
install -m 0644 -D /usr/local/libexec/jumanpp/jumandic.config "$RESDIR/libexec/jumanpp/jumandic.config"

# TODO: when alpine will have GCC 11.*, change architectures to generic ones (x86-64-v*)

# build and install nehalem-compatible version
if [[ $LEVEL -ge 3 ]]; then # sse 4.1
$SHELL "$SCRIPT_DIR/build-pgo.sh" "--arch=nehalem" "--model=$MODEL_FILE" "--data=$TEXT_DATA" -DCMAKE_CXX_FLAGS=-U_FORTIFY_SOURCE
install -m 0755 -D "$JPP_SRC/bld-nehalem/src/jumandic/jumanpp_v2" "$RESDIR/bin/jumanpp-x86-64-v2"
fi

# build and install haswell-compatible version
if [[ $LEVEL -ge 4 ]]; then # avx2/fma
$SHELL "$SCRIPT_DIR/build-pgo.sh" "--arch=haswell" "--model=$MODEL_FILE" "--data=$TEXT_DATA" -DCMAKE_CXX_FLAGS=-U_FORTIFY_SOURCE
install -m 0755 -D "$JPP_SRC/bld-haswell/src/jumandic/jumanpp_v2" "$RESDIR/bin/jumanpp-x86-64-v3"
fi


# finally, avx-512 version
# TODO: enable with GCC 11.*
#if [[ $LEVEL -ge 5 ]]; then # avx-512 compatible
#$SHELL "$SCRIPT_DIR/build-pgo.sh" "--arch=x86-64-v4" "--model=$MODEL_FILE" "--data=$TEXT_DATA" -DCMAKE_CXX_FLAGS=-U_FORTIFY_SOURCE
#install -m 0755 -D "$JPP_SRC/bld-x86-64-v4/src/jumandic/jumanpp_v2" "$RESDIR/bin/jumanpp-x86-64-v4"
#fi

# and finally, runner script
install -m 0755 -D "$SCRIPT_DIR/jumanpp-multiarch.sh" "$RESDIR/bin/jumanpp"
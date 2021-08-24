#!/bin/sh
# Select Juman++ binary which is compiled with extended instruction set

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
RUN_DIR=$(dirname "$FULL_NAME")

if [ $LEVEL -ge 5 ]; then # avx-512 compatible
  ARCH="x86-64-v4"
  EXE="$RUN_DIR/jumanpp-$ARCH"
  if [[ -x "$EXE" ]]; then
    exec "$EXE" "$@"
  fi
fi

if [ $LEVEL -ge 4 ]; then # avx2/fma/bmi2
  ARCH="x86-64-v3"
  EXE="$RUN_DIR/jumanpp-$ARCH"
  if [[ -x "$EXE" ]]; then
    exec "$EXE" "$@"
  fi
fi

if [ $LEVEL -ge 3 ]; then # sse 4.1/popcnt
  ARCH="x86-64-v2"
  EXE="$RUN_DIR/jumanpp-$ARCH"
  if [[ -x "$EXE" ]]; then
    exec "$EXE" "$@"
  fi
fi

EXE="$RUN_DIR/jumanpp-default"
if [[ -x "$EXE" ]]; then
  exec "$EXE" "$@"
fi

echo "Failed to resolve correct jumanpp executable" >&2
exit 1

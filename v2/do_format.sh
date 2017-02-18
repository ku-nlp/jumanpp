#/bin/sh

find -E . -regex '.+\.(cc|cpp|h|hpp)' -print0 | xargs -0 clang-format -i
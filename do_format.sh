#!/bin/sh

#find -E src -regex '.+\.(cc|cpp|h|hpp|proto)' -print0 | xargs -0 clang-format -i
git add .
exec python "$(dirname "$0")/script/git-clang-format.py" "$*"
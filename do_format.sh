#/bin/sh

#find -E . -regex '.+\.(cc|cpp|h|hpp)' -print0 | xargs -0 clang-format -i
git add .
exec python $(dirname $0)/script/git-clang-format.py $*
git add .
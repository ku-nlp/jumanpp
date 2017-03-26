#!/bin/zsh

local -A opthash
zparseopts -D -M -A opthash y

#CMD="cat"
#
#if [[ -n "${opthash[(i)-y]}" ]]; then
#    CMD="sed \"s/_([^_]*)_[^ ]*/_\1/\""
#fi

wdiff -n <(echo ": corpus:$1 ")  <( echo ": output:$2 ") | colordiff
wdiff -n <(cat $1 | sed 's/_\([^_]*\)_\([^_]*\)_\([^_]*\)_[^ ]* /_\1_\3 /g' |sed "s/S-ID:/S-ID /g"|sed "s/^/　 /")  <( cat $2 |sed 's/_\([^_]*\)_\([^_]*\)_\([^_]*\)_[^ ]* /_\1_\3 /g' |sed "s/S-ID:/S-ID /g"|sed "s/^/　 /" ) | colordiff

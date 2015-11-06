#!/bin/zsh
wdiff -n <(echo ": corpus:$1 ")  <( echo ": output:$2 ") | colordiff
wdiff -n <(cat $1 |sed "s/S-ID:/S-ID /g"|sed "s/:[^ ]*//g" |sed "s/^/　 /")  <( cat $2 |sed "s/S-ID:/S-ID /g"|sed "s/:[^ ]*//g"|sed "s/^/　 /" ) | colordiff

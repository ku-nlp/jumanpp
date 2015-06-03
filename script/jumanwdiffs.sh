#!/bin/zsh
wdiff -n <(echo ": corpus:$1 ")  <( echo ": output:$2 ") | colordiff
wdiff -n <(cat $1 |sed "s/^/　 /")  <( cat $2 |sed "s/^/　 /" ) | colordiff

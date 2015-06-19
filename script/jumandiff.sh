#!/bin/zsh

wdiff -3n <(cat $1 |sed "s/:[^ ]*//g" | sed "s/^/　 /" )  <(cat $2 |sed "s/:[^ ]*//g" |sed "s/^/　 /" )|perl -pe "s/\n//"|perl -pe "s/(=+)/\n/g" |sed "s/^ *//"|sort|uniq -c|sort -nr

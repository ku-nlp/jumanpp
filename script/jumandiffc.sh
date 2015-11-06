#!/bin/zsh
wdiff -3n <(cat $1 |sed "s/^/　 /")  <( cat $2 |sed "s/^/　 /" ) |colordiff|perl -pe "s/\n//"|perl -pe "s/(=+)/\n/g" |sed "s/^ *//"|sort|uniq -c|sort -nr

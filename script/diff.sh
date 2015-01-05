#!/bin/sh

wdiff -3n $1 $2 |perl -pe "s/\n//"|perl -pe "s/(=+)/\n/g"|sed "s/^ *//"|sort|uniq -c|sort -nr|colordiff

#!/bin/env python2
#encoding: utf-8

from pyknp import Jumanpp
import sys
import codecs

sys.stdin = codecs.getreader('utf_8')(sys.stdin)
sys.stdout = codecs.getwriter('utf_8')(sys.stdout)

# Use Juman++ in subprocess mode
jumanpp = Jumanpp()
result = jumanpp.analysis(u"ケーキを食べる")
for mrph in result.mrph_list():
    print u"見出し:%s" % (mrph.midasi)


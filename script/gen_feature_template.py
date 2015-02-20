#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
"""
__author__ = 'Yuta Hayashibe' 
__version__ = ""
__copyright__ = ""
__license__ = "GPL v3"


import codecs
import sys
sys.stdin  = codecs.getreader('UTF-8')(sys.stdin)
sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)

FEATURES={ u"表層" : u"w", u"頻度" : u"f", u"品詞" : u"p", u"細分類" : u"sp", u"活用形" : u"sf", u"活用型" : u"sft", u"文字数" : u"l", u"原形" : u"ba" }

def conv(line):
    items = line.split(u"(")
    if len(items[0]) == 0:
        del items[0]

    ret = u""
    if len(items) == 1:
        fnames = items[0][:-1]
        features = []
        for fname in fnames.split(u"-"):
            features.append(u"%" + FEATURES[fname])
        ret = u"UNIGRAM %s:%s" % (line, u",".join(features))
    elif len(items) == 2:
        fnames = items[0][:-1]
        features = []
        for fname in fnames.split(u"-"):
            features.append(u"%L" + FEATURES[fname])

        fnames = items[1][:-1]
        for fname in fnames.split(u"-"):
            features.append(u"%R" + FEATURES[fname])

        ret = u"BIGRAM %s:%s" % (line, u",".join(features))

    else:
        raise "Unkowon size"
    return ret


import optparse
import sys
def main():
    for line in iter(sys.stdin.readline, ""):
        print conv(line.lstrip().rstrip())

if __name__=='__main__':
    main()


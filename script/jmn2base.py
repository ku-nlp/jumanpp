#!/usr/bin/env python
# coding:utf-8

import sys
import codecs
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-p', action='store_true',
                    dest='print_sharp', help='print ID lines.')
args = parser.parse_args()

python_version=sys.version_info[0]
if(python_version == 2):
  sys.stdout = codecs.getwriter('utf_8')(sys.stdout)
  input_iter = codecs.iterdecode(iter(sys.stdin.readline,''), "utf-8")
else:
  input_iter = iter(sys.stdin.readline,'')

sentence = []
for line in input_iter:
  uline = line.rstrip('\n')
  if(uline.startswith('#')):
    if(args.print_sharp):
      print(uline)
  elif(uline.startswith("@")):
    continue
  elif(uline == u'EOS' or uline == u'EOP'):
    print(" ".join(sentence))
    sentence = []
  else:
    sentence.append(uline.split()[2])

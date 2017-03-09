#!/bin/python3
#coding:utf-8

import re
import sys
from sexp import SParser
from idmap import IdMap
import argparse

parser = argparse.ArgumentParser()

# 位置引数
parser.add_argument('--grammar', action='store')
parser.add_argument('--katuyou', action='store')

args = parser.parse_args()

if __name__ == "__main__":
    idmap = IdMap() 
    
    if(args.grammar):
        with open(args.grammar,"r") as grammarf:
            idmap.parse_grammar(grammarf.read())
        for (key1, key2), val in sorted(idmap.pos_spos2id.items(),key=lambda x:idmap.pos2id[x[0][0]]*1000+x[1]):
            print(key1,key2,idmap.pos2id[key1],val)
   
    if(args.katuyou):
        with open(args.katuyou,"r") as katuyouf:
            idmap.parse_katuyou(katuyouf.read())
        for (key1, key2), val in sorted(idmap.type_form2id.items(),key=lambda x:idmap.type2id[x[0][0]]*1000+x[1]):
            print(key1,key2,idmap.type2id[key1],val)

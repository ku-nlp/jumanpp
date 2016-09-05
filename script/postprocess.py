#!/usr/bin/env python3
#coding:utf-8

import sys
import pyknp
import argparse
import itertools
from collections import defaultdict

parser = argparse.ArgumentParser()

# オプション引数
parser.add_argument('--nbest', '-n', default=-1, type=int,
                    dest='nbest',help='Delete paths which have a rank greater than N')
parser.add_argument('--pos', '-p', action='store_true',
                    dest='pos',help='Set nbest for evaluation')
parser.add_argument('--sort', '-s', action='store_true',
                    dest='sort',help='Sort morph by midasi in dictionary order')
parser.add_argument('--sortbyrank', '-m', action='store_true',
                    dest='sortbyrank',help='Sort morph by rank')
parser.add_argument('--color', '-c', action='store_true',
                    dest='hilight', help='Hilight higher rank morph')

args = parser.parse_args()

def iter_EOS(file):
    buf = []
    for line in file:
        buf.append(line)
        if(line.startswith("EOS")):
            yield "".join(buf)
            buf = []

def match_morph(gold, sys, opt):
    sys_hinsi = sys.hinsi
    if(sys.hinsi == "未定義語"):
        sys_hinsi = "名詞"
    correct = (gold.midasi == sys.midasi and
    (gold.hinsi == sys_hinsi or not opt['hinsi']) and
    (gold.bunrui == sys.bunrui or not opt['bunrui']))
    return correct 

def span_match(gold, sys):
    return (mg.span[0] == ms.span[0] and mg.span[1] == ms.span[1])

def span(m):
    return "({},{})".format(m.span[0], m.span[1])

def fval(prec,recall):
    return((prec*recall*2)/(prec+recall))

def rank_check(m, opt):
    for available in opt['available_rank']:
        if(available in m.feature['ランク']):
            return True

def has_higher_rank(m, rank):
    if(not rank > 0):
        return True
    for acceptable_rank in range(1, rank+1):
        if(str(acceptable_rank) in m.feature['ランク']):
            return True

def sort_by_rank(m_list):
    return sorted(m_list, key=lambda m:int(m.feature['ランク'][0]))

def spec(span_list, span2morph):
    for sp in span_list:
        for m in span2morph[sp]:
            if args.hilight and m.feature['ランク'][0] == "1":
                yield(m.new_spec())
            elif(args.hilight):
                yield('\033[32m'+m.new_spec()+'\033[0m')
            else:
                yield(m.new_spec())

# TODO: span の順番を短い順にするオプションを追加する(現在は長い順)
# ex: 1-5, 1-8, 2-5, 2-3,....

for index, string in enumerate(iter_EOS(sys.stdin)):
    try: 
        lattice = pyknp.BList(string, newstyle=True)
    except:
        print('ERROR: parse failed.', file=sys.stderr)
        print(string, file=sys.stderr)
        continue
   
    span2morph = {}
    span_list = []
    for m in lattice.mrph_list():
        if (span(m) not in span2morph):
            span2morph[span(m)] = [m]
            span_list.append(span(m))
        else:
            span2morph[span(m)].append(m)
    for sp in span2morph:
        # rank filter
        if args.nbest > 0:
            mlist = []
            for m in span2morph[sp]:
                if(has_higher_rank(m, args.nbest)):
                    mlist.append(m)
            span2morph[sp] = mlist
        # pos filter
        if args.pos:
            mlist = []
            posmap = defaultdict(list)
            basemap = set()
            for m in sort_by_rank(span2morph[sp]):
                hinsi = m.hinsi
                base = m.genkei
                if(m.hinsi == "形容詞" and m.katuyou2 == "語幹"):
                    hinsi = "名詞"
                if(hinsi not in posmap or 
                        m.feature['ランク'] in posmap[hinsi] or 
                        (base not in basemap and hinsi == "動詞")):
                    posmap[hinsi].append(m.feature['ランク'])
                    basemap.add(base)
                    mlist.append(m)
            span2morph[sp] = mlist
        # sort 
        if args.sort:
            span2morph[sp].sort(key=lambda x:x.midasi)
        elif args.sortbyrank:
            span2morph[sp] = sort_by_rank(span2morph[sp])
    
    # print spec
    out = [lattice.comment]
    for ms in spec(span_list, span2morph):
        out.append(ms)
    out.append('EOS')
    print("".join(out))



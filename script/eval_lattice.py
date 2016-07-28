#!/usr/bin/env python3
#coding:utf-8

import sys
import pyknp
import argparse
import itertools

parser = argparse.ArgumentParser()

# 位置引数
parser.add_argument('gold_input', nargs=1)
parser.add_argument('sys_input', nargs=1)

# オプション引数
parser.add_argument('--nbest', '-n', default=5, type=int,
                    dest='nbest',help='Set nbest for evaluation')
parser.add_argument('--joint', '-j', action='store_true',
                    dest='joint',help='Set nbest for evaluation')
parser.add_argument('--spos', '-s', action='store_true',
                    dest='spos',help='Set nbest for evaluation')
parser.add_argument('--head', action='store', type=int, default=-1,
                    dest='head',help='head')
parser.add_argument('--skip', action='store', type=int, default=-1,
                    dest='skip',help='head')
parser.add_argument('--diff', action='store_true',
                    dest='diff',help='diff')
parser.add_argument('--gtlength', action='store', type=int, default=-1,
                    dest='gtlength',help='length')
parser.add_argument('--ltlength', action='store', type=int, default=-1,
                    dest='ltlength',help='length')

args = parser.parse_args()
opt = {'hinsi':(args.joint or args.spos), 'bunrui':args.spos, 'rank':args.nbest, 'available_rank': [str(x) for x in range(1,args.nbest+1)] }

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

sum_gold = 0
sum_sys = 0
correct = 0
correct_nbest = 0

with open(args.gold_input[0], mode='r', encoding='utf-8') as goldf:
    with open(args.sys_input[0], mode='r', encoding='utf-8') as sysf:
        for index,(gold_lattice, sys_lattice) in enumerate(zip(iter_EOS(goldf),iter_EOS(sysf))):
            if(args.skip >= 0 and args.skip != index):
                continue
            gold = pyknp.BList(gold_lattice, newstyle=True)
            sys = pyknp.BList(sys_lattice, newstyle=True) 

            # 長さチェック
            gold_len = len("".join([m.midasi for m in gold.mrph_list()]))
            if(args.gtlength>=0 and args.ltlength>=0 and 
                    not(args.gtlength <= gold_len < args.ltlength)):
                continue;

            match = set()
            nbest_match = set()
            gold_span = set()
            if(gold.sid != sys.sid):
                print("Error incinsistent sid was given.", file=sys.stderr)
                print("gold:", gold.sid)
                print("sys:", sys.sid)
            
            for mg in gold.mrph_list():
                gold_span.add(span(mg))
            nbest_morph = [m for m in sys.mrph_list() if rank_check(m, opt)]
            span2morph = {}
            span_list = []
            for mg in gold.mrph_list():
                if (span(mg) not in span2morph):
                    span2morph[span(mg)] = mg
                    span_list.append(span(mg))
            for (mg, ms) in itertools.product(gold.mrph_list(), nbest_morph):
                if (span_match(mg,ms) and match_morph(mg,ms,opt)):
                    match.add(span(mg))
                    nbest_match.add(ms)
            
            if(args.diff): # print diff
                count = 0
                for sp in span_list:
                    if(sp in match):
                        print("",span2morph[sp].midasi,":{}".format(count), end="") 
                        count += 1
                    else:
                        print("\033[31m",span2morph[sp].midasi,"\033[0m", end="") 
                print()
            
            correct += len(match)
            correct_nbest += len(nbest_match)
            sum_sys += len(nbest_morph)
            sum_gold += len(gold_span)
            if(index == args.head-1):
                break


print("Recall: {:02.4} ({}/{})".format(correct/sum_gold, correct, sum_gold))
print("Precision: {:02.4} ({}/{})".format(correct_nbest/sum_sys, correct_nbest, sum_sys))
print("F-val: {:02.4}".format(fval(correct/sum_gold, correct_nbest/sum_sys)))
#print("{:02.4} {:02.4} {:02.4}".format(correct/sum_gold, correct_nbest/sum_sys,fval(correct/sum_gold, correct_nbest/sum_sys) ))


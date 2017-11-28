#!/usr/bin/env python3

# Converts a Fairy Morphological Annotated Corpus to Juman++ partial annotated format

import argparse
import sys


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument("--input")
    p.add_argument("--output", default='-')
    return p.parse_args()


def parse_sentence(line, out):
    lastx = line.rfind('|')
    inside = False
    stop = len(line)
    resline = ""
    maybe_cut = False
    for i in range(stop):
        c = line[i]
        if c == '|':
            if len(resline) > 0:
                if maybe_cut:
                    out.write(resline)
                elif inside:
                    out.write('\t')
                    out.write(resline.replace('&', ''))
                else:
                    out.write(resline)
                out.write('\n')
                maybe_cut = False
                resline = ""
            inside = i < lastx
        elif c == '?':
            maybe_cut = True
        else:
            resline += c
            if i == stop - 1:
                continue
            nextChar = line[i + 1]
            if nextChar == '|':
                continue
            if inside and nextChar != '?':
                resline += '&'
    if len(resline) > 0:
        out.write(resline)
        out.write('\n\n')
    else:
        out.write("\n")


def main(inf, outf, args):
    for lineno, line in enumerate(inf):
        output.write(f"# FairyMa-{lineno}\n")
        tabidx = line.find('\t')
        if tabidx != -1:
            line = line[0:tabidx]
        else:
            line = line.strip()

        parse_sentence(line, outf)


if __name__ == '__main__':
    arg = parse_args()
    if arg.output == '-':
        output = sys.stdout
    else:
        output = open(arg.output, 'wt', encoding='utf-8')
    with open(arg.input, 'rt', encoding='utf-8') as inf:
        main(inf, output, arg)
    output.close()

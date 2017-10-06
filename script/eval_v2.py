from sys import stderr, stdout
import argparse
import re

class DiffPart(object):
    EQUAL = 0
    DIFF0 = 1
    DIFF1 = 2
    DIFF2 = 3

    def __init__(self, kind, syst, gold):
        self.kind = kind
        self.sys = syst
        self.gold = gold

    def __repr__(self):
        return f"{self.kind} {self.sys} {self.gold}"


class ScoreInfo(object):
    def __init__(self):
        self.tp = [0, 0, 0]
        self.fp = [0, 0, 0]
        self.fn = [0, 0, 0]

    def add_diff(self, diff):
        for p in diff:
            sys_cnt = len(p.sys)
            gold_cnt = len(p.gold)
            if p.kind == DiffPart.EQUAL:
                self.tp[0] += sys_cnt
                self.tp[1] += sys_cnt
                self.tp[2] += sys_cnt
            elif p.kind == DiffPart.DIFF2:
                self.tp[0] += sys_cnt
                self.tp[1] += sys_cnt
                self.fp[2] += sys_cnt
                self.fn[2] += gold_cnt
            elif p.kind == DiffPart.DIFF1:
                self.tp[0] += sys_cnt
                self.fp[1] += sys_cnt
                self.fn[1] += gold_cnt
                self.fp[2] += sys_cnt
                self.fn[2] += gold_cnt
            else:
                self.fp[0] += sys_cnt
                self.fn[0] += gold_cnt
                self.fp[1] += sys_cnt
                self.fn[1] += gold_cnt
                self.fp[2] += sys_cnt
                self.fn[2] += gold_cnt

    def stats(self):
        prec = [float(tp)/(tp + fp) for tp, fp in zip(self.tp, self.fp)]
        recall = [float(tp)/(tp + fn) for tp, fn in zip(self.tp, self.fn)]
        f1 = [(2 * p * r)/(p + r) for p, r in zip(prec, recall)]
        return prec, recall, f1


yellow = '\033[0;33m'
red = '\033[0;31m'
green = '\033[0;32m'
blue = '\033[0;34m'
reset = '\033[0;0m'

def render_diff(diff, buffer):
    has_space = False
    for p in diff:
        if p.kind == DiffPart.EQUAL:
            buffer.write("".join([x[0] for x in p.sys]))
            has_space = False
        elif p.kind == DiffPart.DIFF2:
            if not has_space:
                buffer.write(" ")
            for sys, gold in zip(p.sys, p.gold):
                buffer.write(f"{sys[0]}_{sys[1]}:[{blue}{sys[2]}{reset}/{green}{gold[2]}{reset}] ")
            has_space = True
        elif p.kind == DiffPart.DIFF1:
            if not has_space:
                buffer.write(" ")
            for sys, gold in zip(p.sys, p.gold):
                buffer.write(f"{sys[0]}_[{yellow}{sys[1]}:{sys[2]}{reset}/{green}{gold[1]}:{gold[2]}{reset}] ")
            has_space = True
        else:
            if not has_space:
                buffer.write(" ")
            buffer.write("[ ")
            buffer.write(red)
            for sys in p.sys:
                buffer.write(f"{sys[0]}_{sys[1]}:{sys[2]} ")
            buffer.write(reset)
            buffer.write("/ ")
            buffer.write(green)
            for gold in p.gold:
                buffer.write(f"{gold[0]}_{gold[1]}:{gold[2]} ")
            buffer.write(reset)
            buffer.write("] ")
            has_space = True
    buffer.write('\n')


class Diff(object):
    __slots__ = ['data']

    def __init__(self):
        self.data = []

    def __index__(self, i):
        return self.data[i]

    def append(self, item: DiffPart):
        d = self.data
        if len(d) == 0:
            d.append(item)
            return

        last = d[-1]
        if last.kind == item.kind:
            last.sys.extend(item.sys)
            last.gold.extend(item.gold)
        else:
            d.append(item)

    def __iter__(self):
        return self.data.__iter__()


def compute_diff(sysobj, goldobj, lineno):
    sysidx = 0
    goldidx = 0
    sysparts = sysobj.parts
    syslen = len(sysparts)
    goldparts = goldobj.parts
    goldlen = len(goldparts)
    syschar = 0
    goldchar = 0

    diff = Diff()

    while sysidx < syslen and goldidx < goldlen:
        sys = sysparts[sysidx]
        gold = goldparts[goldidx]
        gold_surflen = len(gold[0])
        sys_surflen = len(sys[0])

        if gold_surflen == sys_surflen:

            if sys[0] != gold[0]:
                kind = DiffPart.DIFF0
            elif sys[1] == '未定義語' and gold[1] == '名詞':
                kind = DiffPart.DIFF2
            elif sys[1] != gold[1]:
                kind = DiffPart.DIFF1
            elif sys[2] != gold[2]:
                kind = DiffPart.DIFF2
            else:
                kind = DiffPart.EQUAL

            diff.append(DiffPart(kind, [sys], [gold]))
            syschar += sys_surflen
            goldchar += gold_surflen
            sysidx += 1
            goldidx += 1
        else:
            sysstart = sysidx
            goldstart = goldidx
            syschar += sys_surflen
            goldchar += gold_surflen
            sysidx += 1
            goldidx += 1

            while syschar != goldchar:
                if syschar > goldchar:
                    if goldidx < goldlen:
                        goldchar += len(goldparts[goldidx][0])
                        goldidx += 1
                    else:
                        break
                else:
                    if sysidx < syslen:
                        syschar += len(sysparts[sysidx][0])
                        sysidx += 1
                    else:
                        break

            sysappd = sysparts[sysstart:sysidx]
            goldappd = goldparts[goldstart:goldidx]
            diff.append(DiffPart(DiffPart.DIFF0, sysappd, goldappd))

    if sysidx < syslen:
        diff.append(DiffPart(DiffPart.DIFF0, sysparts[sysidx:], []))
    if goldidx < goldlen:
        diff.append(DiffPart(DiffPart.DIFF0, [], goldparts[goldidx:]))

    return diff


def not_same(diff):
    for p in diff:
        if p.kind != DiffPart.EQUAL:
            return True
    return False


def calculate_stats(syslines, goldlines, args):
    line = 1
    scores = ScoreInfo()

    if len(goldlines) != len(syslines):
        print("System output and gold data have different number of sentences", file=stderr)
        exit(1)

    for golddata, sysdata in zip(goldlines, syslines):
        sysobj = parse_sentence(sysdata.strip(), line)
        goldobj = parse_sentence(golddata.strip(), line)
        diff = compute_diff(sysobj, goldobj, line)
        scores.add_diff(diff)
        if args.verbose and not_same(diff):
            if goldobj.comment is not None:
                print(goldobj.comment)
            elif sysobj.comment is not None:
                print(sysobj.comment)
            render_diff(diff, stdout)

        line += 1
    return scores


class Sentence(object):
    __slots__ = ['comment', 'parts']

    def __init__(self, comment, parts):
        self.comment = comment
        self.parts = parts


def parse_sentence(line, lineno):
    idx = line.find('#')
    if idx == -1:
        comment = None
    else:
        comment = line[idx:]
        line = line[:idx].strip()

    parts = line.split(' ')
    try:
        return Sentence(comment, [separate_parts(p) for p in parts])
    except ValueError:
        print("Failed to parse line ", lineno, line, file=stderr)
        exit(1)


def read_and_eval(opts):
    with open(opts.system, 'rt') as sysin:
        with open(opts.gold, 'rt') as goldin:
            return calculate_stats(
                syslines=sysin.readlines(),
                goldlines=goldin.readlines(),
                args=opts)

parts_regex = re.compile(r'^([^_]+)_([^:]+):(.*)$')

def separate_parts(obj):
    mtch = parts_regex.fullmatch(obj)
    if mtch is None:
        raise ValueError()
    return mtch.groups()


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('system')
    p.add_argument('gold')
    p.add_argument('-v', '--verbose', action='store_true')
    return p.parse_args()


if __name__ == '__main__':
    stats = read_and_eval(parse_args())
    seg, joint, sjoint = stats.stats()
    print(f"Seg: ({stats.tp[0]:5}/{stats.tp[0] + stats.fp[0]:5}({seg[0]:3.2F}")

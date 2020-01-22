from sys import stderr, stdout
import argparse
import re
import random
import math

try:
    import numpy as np

    no_numpy = False
except ImportError:
    no_numpy = True


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


class StreamingVariance(object):
    def __init__(self):
        self.cnt = 0
        self.m2 = 0
        self.mean = 0

    def add(self, x):
        self.cnt += 1
        delta = x - self.mean
        self.mean += delta / self.cnt
        delta2 = x - self.mean
        self.m2 += delta * delta2

    def variance(self):
        return self.m2 / self.cnt


class Measure(object):
    __slots__ = ["fp", "tp", "fn", "prec", "rec", "f1"]

    def __init__(self, tp, fp, fn):
        """
        :type tp: int
        :type fp: int
        :type fn: int
        """
        self.tp = tp
        self.fp = fp
        self.fn = fn
        self.prec = tp / max(tp + fp, 1)
        self.rec = tp / max(tp + fn, 1)
        self.f1 = 2 * self.prec * self.rec / max(self.prec + self.rec, 1e-6)

    def prec_str(self):
        return f"{self.prec * 100:.4F} ({self.tp}/{self.tp + self.fp})"

    def rec_str(self):
        return f"{self.rec * 100:.4F} ({self.tp}/{self.tp + self.fn})"


class Measure2(object):
    def __init__(self):
        self.tp = StreamingVariance()
        self.fp = StreamingVariance()
        self.fn = StreamingVariance()
        self.prec = StreamingVariance()
        self.rec = StreamingVariance()
        self.f1 = StreamingVariance()

    def add(self, x):
        self.tp.add(float(x.tp))
        self.fp.add(float(x.fp))
        self.fn.add(float(x.fn))
        self.prec.add(x.prec)
        self.rec.add(x.rec)
        self.f1.add(x.f1)


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

    def add(self, o):
        for i in range(3):
            self.tp[i] += o.tp[i]
            self.fp[i] += o.fp[i]
            self.fn[i] += o.fn[i]

    def stats(self):
        measures = [Measure(*args) for args in zip(self.tp, self.fp, self.fn)]
        return measures


yellow = '\033[0;33m'
red = '\033[0;31m'
green = '\033[0;32m'
blue = '\033[0;34m'
reset = '\033[0;0m'


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
        return iter(self.data)

    def __len__(self):
        return len(self.data)


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


class Bootstrap(object):
    def __init__(self):
        self.scores = []
        self.normal = ScoreInfo()

    def add_diff(self, diff):
        s = ScoreInfo()
        s.add_diff(diff)
        self.normal.add_diff(diff)
        self.scores.append(s)

    def run(self, niters):
        stats = [Measure2(), Measure2(), Measure2()]
        for i in range(niters):
            s = self.resample_iter()
            ms = s.stats()
            print(i, ms[0].prec_str())
            for a, b in zip(stats, ms):
                a.add(b)
        return stats

    def resample_iter(self):
        s = ScoreInfo()
        scores = self.scores
        max_score = len(scores)
        for i in range(max_score):
            idx = random.randrange(max_score)
            so = scores[idx]
            s.add(so)
        return s


def print_bootstrap(bootstrap, niters):
    stats = bootstrap.run(niters)
    s0 = bootstrap.normal.stats()
    seg = stats[0]
    segn = s0[0]
    variance = seg.prec.variance()
    serr = math.sqrt(variance)
    cb = serr / math.sqrt(niters) * 3.291
    mean = seg.prec.mean
    data = [mean, variance, mean - cb, mean + cb, cb]
    strs = [format(x * 100, ".4F") for x in data]
    print(",".join(strs), segn.prec_str(), sep=",")


def calculate_stats(syslines, goldlines, args):
    line = 1
    if args.bootstrap is None:
        scores = ScoreInfo()
    else:
        scores = Bootstrap()

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
    with open(opts.system, 'rt', encoding='utf-8') as sysin:
        with open(opts.gold, 'rt', encoding='utf-8') as goldin:
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
    p.add_argument('-b', '--bootstrap', type=int)
    return p.parse_args()


if __name__ == '__main__':
    args = parse_args()
    stats = read_and_eval(args)
    if args.bootstrap is None:
        ms = stats.stats()
        seg = ms[0]
        print(f"Seg: {seg.prec_str()} {seg.rec_str()} {seg.f1 * 100:.2F}", file=stderr)
    else:
        print_bootstrap(stats, args.bootstrap)

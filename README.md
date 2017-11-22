# JUMAN++
-----
A new morphological analyser that considers semantic plausibility of 
word sequences by using a recurrent neural network language model (RNNLM).
Version 2 has better accuracy and improved (>100x) performance than
the original JUMAN++.

# Notice

This is a branch for the Juman++ rewrite.
The original version lives in the [master](https://github.com/ku-nlp/jumanpp/tree/master) branch.
How to compile development version:

### Requirements for JUMAN++ (v2)

- OS: Linux or MacOS X
- Compiler: C++14 compatible (will [downgrade to C++11](https://github.com/ku-nlp/jumanpp/issues/20) later)
-- We test on GCC and clang
- CMake v3.1 or later

```bash
$ mkdir cmake-build-dir # CMake does not support in-source builds
$ cd cmake-build-dir
$ cmake ..
$ make
```

Prelearned models are not yet available, but they will be very soon.

## Quick start
```
% echo "魅力がたっぷりと詰まっている" | jumanpp
魅力 みりょく 魅力 名詞 6 普通名詞 1 * 0 * 0 "代表表記:魅力/みりょく カテゴリ:抽象物"
が が が 助詞 9 格助詞 1 * 0 * 0 NIL
たっぷり たっぷり たっぷり 副詞 8 * 0 * 0 * 0 "自動認識"
と と と 助詞 9 格助詞 1 * 0 * 0 NIL
詰まって つまって 詰まる 動詞 2 * 0 子音動詞ラ行 10 タ系連用テ形 14 "代表表記:詰まる/つまる ドメイン:料理・食事 自他動詞:他:詰める/つめる"
いる いる いる 接尾辞 14 動詞性接尾辞 7 母音動詞 1 基本形 2 "代表表記:いる/いる"
EOS
```

## Option
```
usage: jumanpp [options] 
options:
  -s, --specifics              lattice format output (unsigned int [=5])
  -B, --beam                   set beam width used in analysis (unsigned int [=5])
  -v, --version                print version
  -h, --help                   print this message
```

## Input
JUMAN++ can handle only utf-8 encoded text as an input.
Lines beginning with `# ` will be interpreted as comments.

## DEMO
You can play around our [web demo](http://tulip.kuee.kyoto-u.ac.jp/demo/jumanpp_lattice?text=%E5%A4%96%E5%9B%BD%E4%BA%BA%E5%8F%82%E6%94%BF%E6%A8%A9%E3%81%AB%E5%AF%BE%E3%81%99%E3%82%8B%E8%80%83%E3%81%88%E6%96%B9%E3%81%AE%E9%81%95%E3%81%84)
which displays a lattice

## Model

See Morphological Analysis for Unsegmented Languages using Recurrent Neural Network Language Model. *Hajime Morita, Daisuke Kawahara, Sadao Kurohashi*. EMNLP 2015 [link](http://aclweb.org/anthology/D/D15/D15-1276.pdf).


## Authors
* Arseny Tolmachev <arseny **at** kotonoha.ws>
* Hajime Morita <hmorita  **at** i.kyoto-u.ac.jp>  
* Daisuke Kawahara <dk  **at** i.kyoto-u.ac.jp>  
* Sadao Kurohashi <kuro  **at** i.kyoto-u.ac.jp>

## Acknowledgement
The list of all libraries used by JUMAN++ is [here](libs/README.md).


----

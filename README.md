# What is Juman++

A new morphological analyser that considers semantic plausibility of 
word sequences by using a recurrent neural network language model (RNNLM).
Version 2 has better accuracy and greatly (>250x) improved analysis speed than
the original Juman++.

[![Build Status](https://travis-ci.org/ku-nlp/jumanpp.svg?branch=master)](https://travis-ci.org/ku-nlp/jumanpp)

# Installation

## System Requirements

* OS: Linux, MacOS X or Windows.
* Compiler: C++14 compatible (will [downgrade to C++11](https://github.com/ku-nlp/jumanpp/issues/20) later)
  * For, example gcc 5.1+, clang 3.4+, MSVC 2017
  * We test on GCC and clang on Linux/MacOS, mingw64-gcc and MSVC2017 on Windows
- CMake v3.1 or later

## Building from a package

Download the package from [Releases](https://github.com/ku-nlp/jumanpp/releases)

```bash
$ tar xf jumanpp-<version>.tar.xz # decompress the package
$ cd jumanpp-<version> # move into the directory
$ mkdir bld # make a subdirectory for build
$ cd bld
$ cmake .. \
  -DCMAKE_BUILD_TYPE=Release \ # you want to do this for performance
  -DCMAKE_INSTALL_PREFIX=<prefix> # where to install Juman++
$ make install -j<parallelism>
```

## Building from git

Generally, the differences between the package
and this repository is the presence of a prebuilt model
and absense of some development scripts.

```bash
$ mkdir cmake-build-dir # CMake does not support in-source builds
$ cd cmake-build-dir
$ cmake ..
$ make # -j
```

# Usage

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

## Main options
```
usage: jumanpp [options] 
  -s, --specifics              lattice format output (unsigned int [=5])
  --beam <int>                 set beam width used in analysis (unsigned int [=5])
  -v, --version                print version
  -h, --help                   print this message
  --model <file>               specify a model location
```

More complete description of all options will come later.

## Input
JUMAN++ can handle only utf-8 encoded text as an input.
Lines beginning with `# ` will be interpreted as comments.

# Other

## DEMO
You can play around our [web demo](https://tulip.kuee.kyoto-u.ac.jp/demo/jumanpp_lattice?text=%E5%A4%96%E5%9B%BD%E4%BA%BA%E5%8F%82%E6%94%BF%E6%A8%A9%E3%81%AB%E5%AF%BE%E3%81%99%E3%82%8B%E8%80%83%E3%81%88%E6%96%B9%E3%81%AE%E9%81%95%E3%81%84)
which displays a subset of the whole lattice.
The demo still uses v1 but, it will be updated to v2 soon.

## Performance Notes

To get the best performance, you need to build with [extended instructuion sets](https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html).
If you are planning to use Juman++ only locally,
specify `-DCMAKE_CXX_FLAGS="-march=native"`.

Works best on Intel Haswell and newer processors (because of FMA and BMI instruction set extensions).

## Publications and Slides

* About the model itself: *Morphological Analysis for Unsegmented Languages using Recurrent Neural Network Language Model*. Hajime Morita, Daisuke Kawahara, Sadao Kurohashi. EMNLP 2015 [link](http://aclweb.org/anthology/D/D15/D15-1276.pdf).

* V2 Improvments: *Juman++ v2: A Practical and Modern Morphological Analyzer*. Arseny Tolmachev and Kurohashi Sadao. The Proceedings of the Twenty-fourth Annual Meeting of the Association for Natural Language Processing. March 2018, Okayama, Japan. ([pdf](http://www.anlp.jp/proceedings/annual_meeting/2018/pdf_dir/A5-2.pdf), [slides](https://www.slideshare.net/eiennohito/juman-v2-a-practical-and-modern-morphological-analyzer)

* Morphological Analysis Workshop in ANLP2018 Slides: 形態素解析システムJuman++. 河原 大輔, Arseny Tolmachev. (in Japanese) [slides](https://drive.google.com/file/d/1DVnrsWw4skRgC8jU6_RkeofOQEHFwctc/view?usp=sharing).

## Authors
* Arseny Tolmachev <arseny **at** kotonoha.ws>
* Hajime Morita <hmorita  **at** i.kyoto-u.ac.jp>  
* Daisuke Kawahara <dk  **at** i.kyoto-u.ac.jp>  
* Sadao Kurohashi <kuro  **at** i.kyoto-u.ac.jp>

## Acknowledgement
The list of all libraries used by JUMAN++ is [here](libs/README.md).

## Notice

This is a branch for the Juman++ rewrite.
The original version lives in the [legacy](https://github.com/ku-nlp/jumanpp/tree/legacy) branch.

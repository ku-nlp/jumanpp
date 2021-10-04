# What is Juman++

A new morphological analyser that considers semantic plausibility of 
word sequences by using a recurrent neural network language model (RNNLM).
Version 2 has better accuracy and greatly (>250x) improved analysis speed than
the original Juman++.

[![Build Status](https://github.com/ku-nlp/jumanpp/actions/workflows/cmake.yml/badge.svg)](https://github.com/ku-nlp/jumanpp/actions/workflows/cmake.yml)

# Installation

## System Requirements

* OS: Linux, MacOS X or Windows.
* Compiler: C++14 compatible
  * For example gcc 5.1+, clang 3.4+, MSVC 2017
  * We test on GCC and clang on Linux/MacOS, mingw64-gcc and MSVC2017 on Windows
- CMake v3.1 or later

Read [this document](docs/building.md) for CentOS and RHEL derivatives or non-CMake alternatives.

## Building from a package

Download the package from [Releases](https://github.com/ku-nlp/jumanpp/releases)

**Important**: The download should be around 300 MB. If it is not you have probably downloaded a source snapshot which does not contain a model.

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

**Important**: Only the package distribution contains a pretrained model and can be used for analysis. 
The current git version is not compatible with the models of 2.0-rc1 and 2.0-rc2.

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
  --beam <int>                 set local beam width used in analysis (unsigned int [=5])
  -v, --version                print version
  -h, --help                   print this message
  --model <file>               specify a model location
```

Use `--help` to see more options.

## Input
JUMAN++ can handle only utf-8 encoded text as an input.
Lines beginning with `# ` will be interpreted as comments.

## Training Jumandic Model

A set of scripts for training Jumandic model is available in [this repository](https://github.com/ku-nlp/jumanpp-jumandic).
It is possible to modify the system dictionary to add other entries to the trained model.

**Attention**: You need to have access to Mainichi Shinbun for Year 1995 to be able to use Kyoto Univeristy corpus for training.

# Other

## DEMO
You can play around our [web demo](https://tulip.kuee.kyoto-u.ac.jp/demo/jumanpp_lattice?text=%E5%A4%96%E5%9B%BD%E4%BA%BA%E5%8F%82%E6%94%BF%E6%A8%A9%E3%81%AB%E5%AF%BE%E3%81%99%E3%82%8B%E8%80%83%E3%81%88%E6%96%B9%E3%81%AE%E9%81%95%E3%81%84)
which displays a subset of the whole lattice.
The demo still uses v1 but, it will be updated to v2 soon.

## Extracting diffs caused by beam configurations

You can see sentences in which two different beam configurations produce different analyses.
A `src/jumandic/jpp_jumandic_pathdiff` binary [(source)](https://github.com/ku-nlp/jumanpp/blob/master/src/jumandic/main/path_diff.cc) 
(relative to a compilation root) does it.
The only Jumandic-specific thing here is the usage of [code-generated linear model inference](https://github.com/ku-nlp/jumanpp/blob/master/src/jumandic/main/path_diff.cc#L195).

Use the binary as `jpp_jumandic_pathdiff <model> <input> > <output>`.

Outputs would be in the partial annotation format with a full beam results being the actual tags and trimmed beam results being written as comments.

Example: 
```
# scores: -0.602687 -1.20004
# 子がい        pos:名詞        subpos:普通名詞 <------- trimmed beam result
# S-ID:w201007-0080605751-6 COUNT:2
熊本選抜にはマリノス、アントラーズのユースに行く
        子      pos:名詞        subpos:普通名詞 <------- full beam result
        が      pos:助詞        subpos:格助詞
        い      baseform:いる   conjtype:母音動詞       pos:動詞        conjform:基本連用形
ます
```

## Partial Annotation Tool

We also have a partial annotation tool. Please see https://github.com/eiennohito/nlp-tools-demo for details.

## Performance Notes

To get the best performance, you need to build with [extended instruction sets](https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html).
If you are planning to use Juman++ only locally,
specify `-DCMAKE_CXX_FLAGS="-march=native"`.

Works best on Intel Haswell and newer processors (because of FMA and BMI instruction set extensions).

## Using Juman++ to create your own Morphological Analyzer

Juman++ is a general tool. 
It does not depend on Jumandic or Japanese Language 
(albeit there are some Japanese-specific functionality).
See [this tutorial project](https://github.com/eiennohito/jumanpp-t9)
which shows how to implement a something similar to a 
[T9 text input](https://en.wikipedia.org/wiki/T9_(predictive_text))
for the case when there are no word boundaries in the input text.

## Publications and Slides

* About the model itself: *Morphological Analysis for Unsegmented Languages using Recurrent Neural Network Language Model*. Hajime Morita, Daisuke Kawahara, Sadao Kurohashi. EMNLP 2015 [link](http://aclweb.org/anthology/D15-1276), [bibtex](https://www.aclweb.org/anthology/D15-1276.bib).

* V2 Improvments: *Juman++ v2: A Practical and Modern Morphological Analyzer*. Arseny Tolmachev and Kurohashi Sadao. The Proceedings of the Twenty-fourth Annual Meeting of the Association for Natural Language Processing. March 2018, Okayama, Japan. ([pdf](http://www.anlp.jp/proceedings/annual_meeting/2018/pdf_dir/A5-2.pdf), [slides](https://www.slideshare.net/eiennohito/juman-v2-a-practical-and-modern-morphological-analyzer))

* Morphological Analysis Workshop in ANLP2018 Slides: 形態素解析システムJuman++. 河原 大輔, Arseny Tolmachev. (in Japanese) [slides](https://drive.google.com/file/d/1DVnrsWw4skRgC8jU6_RkeofOQEHFwctc/view?usp=sharing).

* *Juman++: A Morphological Analysis Toolkit for Scriptio Continua.* Arseny Tolmachev, Daisuke Kawahara and Sadao Kurohashi. EMNLP 2018, Brussels. [pdf](http://aclweb.org/anthology/D18-2010), [poster](https://drive.google.com/file/d/1SeiYP2z4Hb1Q0IhIe_3ydSIjCMhxnKIS/view?usp=sharing), [bibtex](https://www.aclweb.org/anthology/D18-2010.bib).

* *Design and Structure of The Juman++ Morphological Analyzer Toolkit.* Arseny Tolmachev, Daisuke Kawahara, Sadao Kurohashi. Journal of Natural Language Processing, ([paper](https://www.jstage.jst.go.jp/article/jnlp/27/1/27_89/_article/-char/en), [bibtex](https://www.jstage.jst.go.jp/AF06S010ShoshJkuDld?sryCd=jnlp&noVol=27&noIssue=1&kijiCd=27_89&kijiLangKrke=en&kijiToolIdHkwtsh=AT0073&request_locale=EN)).

If you use Juman++ V1 in academic setting, then please cite the first work (EMNLP2015). If you use Juman++ V2, then please cite both the first and the fourth (EMNLP2018) papers. 

## Authors
* Arseny Tolmachev <arseny **at** kotonoha.ws>
* Hajime Morita <hmorita  **at** nlp.ist.i.kyoto-u.ac.jp>  
* Daisuke Kawahara <dk  **at** i.kyoto-u.ac.jp>  
* Sadao Kurohashi <kuro  **at** i.kyoto-u.ac.jp>

## Acknowledgement
The list of all libraries used by JUMAN++ is [here](libs/README.md).

## Notice

This is a branch for the Juman++ rewrite.
The original version lives in the [legacy](https://github.com/ku-nlp/jumanpp/tree/legacy) branch.

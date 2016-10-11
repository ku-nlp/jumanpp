# JUMAN++
-----
A new morphological analyser that considers semantic plausibility of 
word sequences by using a recurrent neural network language model (RNNLM).

## Installation
### Requirement
- OS: Linux (tested on CentOS 6.7, Ubuntu 16.4) 
- gcc (4.9 or later)
- Boost C++ Libraries (1.57 or later)  
 http://www.boost.org/users/history/version_1_57_0.html
[]( for unordered_map, interprocess(dynamic loading))
- gperftool (optional)
 https://github.com/gperftools/gperftools
    * libunwind (required by gperftool in 64bit environment) 
    http://download.savannah.gnu.org/releases/libunwind/libunwind-0.99-beta.tar.gz
[Note contains instruction for install libraries.](#markdown-header-note)

### Build
```
% wget http://lotus.kuee.kyoto-u.ac.jp/nl-resource/jumanpp/jumanpp-1.01.tar.xz
% tar xJvf jumanpp-1.01.tar.xz
% cd jumanpp-1.01
% ./configure 
% make
% sudo make install
```
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
It receives utf-8 encoding text as an input.
Lines beginning with `#` will be interpreted as comment line.

## DEMO
[DEMO cgi](http://tulip.kuee.kyoto-u.ac.jp/demo/jumanpp_lattice?text=%E5%A4%96%E5%9B%BD%E4%BA%BA%E5%8F%82%E6%94%BF%E6%A8%A9%E3%81%AB%E5%AF%BE%E3%81%99%E3%82%8B%E8%80%83%E3%81%88%E6%96%B9%E3%81%AE%E9%81%95%E3%81%84)

## Model
See ``Morphological Analysis for Unsegmented Languages using Recurrent Neural Network Language Model. Hajime Morita, Daisuke Kawahara, Sadao Kurohashi. EMNLP 2015''  
[link](http://aclweb.org/anthology/D/D15/D15-1276.pdf)

## Authors
Hajime Morita <hmorita@i.kyoto-u.ac.jp>  
Daisuke Kawahara <dk@i.kyoto-u.ac.jp>  
Sadao Kurohashi <kuro@i.kyoto-u.ac.jp>

## Acknowledgement
Juman++ uses the following open source software/codes:
- [faster-rnnlm](https://github.com/yandex/faster-rnnlm) for training RNNLM and reading models.
- [RNNLM-toolkit](http://rnnlm.org/) for reading RNNLMs.
- [Darts](http://chasen.org/~taku/software/darts/) for Double-Array.
- [tinycdb](http://www.corpit.ru/mjt/tinycdb.html) for reading CDB.
- [cmdline](https://github.com/tanakh/cmdline) by Hideyuki Tanaka to parse command line options.

## Note 

### Install Requirements
#### libunwind
```
wget http://download.savannah.gnu.org/releases/libunwind/libunwind-0.99-beta.tar.gz
tar xzf libunwind-0.99-beta.tar.gz
cd libunwind-0.99-beta
./configure --prefix=/somewhere/local/
make 
make install
```
#### gperftools
```
./configure --prefx=/somewhere/local/
make
# When ld try to link libunwind.so (and failed to build), 
# please set an option "UNWIND_LIBS=-lunwind-x86_64" to make.
make install
```
#### Boost Libraries 
```
sh bootstrap.sh
./b2 install -j2 --prefix=/somewhere/local/
```
----

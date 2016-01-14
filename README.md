---------
# KKN
---------

This is a new morphological analyser that considers semantic plausibility of 
word sequences by using a recurrent neural network language model (RNNLM).

## DEMO
----
http://lotus.kuee.kyoto-u.ac.jp/~morita/rnnlm.cgi

## Installation
-----
### Required libraries
* gperftool
 https://github.com/gperftools/gperftools
** libunwind (required by gperftool in 64bit environment) 
 http://download.savannah.gnu.org/releases/libunwind/libunwind-0.99-beta.tar.gz
* Boost C++ Libraries (1.57 or later)
 http://www.boost.org/users/history/version_1_57_0.html
[]( for serialization, unordered_map, hashing, interprocess(dynamic loading))

### Build
-----
git clone git@bitbucket.org:ku_nlp/kkn.git
make

## Quick start
-----
 ./kkn -D /share/usr-x86_64/tool/kkn/model/latest

## Algorithm 
-----
See ``Morphological Analysis for Unsegmented Languages using Recurrent Neural Network Language Model. Hajime Morita, Daisuke Kawahara, Sadao Kurohashi. EMNLP 2015''

## Authors
-----

Hajime Morita <hmorita@i.kyoto-u.ac.jp>
Daisuke Kawahara <dk@i.kyoto-u.ac.jp>
Sadao Kurohashi <kuro@i.kyoto-u.ac.jp>

## Note
----

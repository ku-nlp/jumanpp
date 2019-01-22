Juman++ supports several output formats.
They are the following:
```
        -j, --juman                  Juman style (default)
        -M, --morph                  Morph style
        -F, --full-morph             Full-morph style
        --dic-subset                 Subset of dictionary, useful for tests
        -L[N], -s[N], --lattice=[N], --specifics=[N]
                                     Lattice style (N-best)
        --segment                    Only segment
        --segment-separator=[SEPARATOR]
                                     Separator for segmented output (default: space)
        --graphviz-dir=[DIRECTORY]
                                     Directory for GraphViz .dot files output
```

There are two types of output formats:
text-based and protobuf-based (binary).
We recommend using text-based formats when the output should be human-readable
and protobuf-based formats when the Juman++ output is consumed by another tool.

### Jumandic Part of Speech format

Jumandic (Kyoto University Kurohashi-Kawahara lab analysis dictionary and segmentation standard)
has 4-layered part of speech (POS) tags.
Those layers are:
1. Rough POS (大分類)
2. Fine POS (細分類)
3. Conjugation Type (活用型)
4. Conjugation Form (活用形)

POS tags can be encoded with strings, for example 接尾辞-形容詞性述語接尾辞-イ形容詞アウオ段-基本形.
Strings are stable and will never change between releases. 
Also, they can also be encoded with ids (14-5-18-2).
Ids are **not stable** and generally can change between releases, but we try not to change them much.
Most of output formats contain both representations.

Astersk charaters (`*`) in POS tag string representation (e.g. `名詞-人名-*-*`) mean that
the corresponding layer is not applicable for the POS.
This situation is endoded by 0 in id representation.

### Comments

Each sentence can be prefixed by a comment.
Comment starts with "# " (sharp character followed by space) and ends with the end of line.
Comments are copied into the output.

## Text-based formats

These formats are useful for human consumption or 
if it is possible to make sure that the input never contains some characters.
Please refer to individual format descriptions for those limitations.
We recommend using Lattice format instead of Juman-based one.
By setting top-N to 1 you will get only top-1 result.

### Juman

Activated by `--juman` or `-j`.
Is a default output format.
Compatible with the default format of [JUMAN](http://nlp.ist.i.kyoto-u.ac.jp/index.php?JUMAN) morphological analyzer.

```
# COMMENT GOES HERE
田中 たなか 田中 名詞 6 人名 5 * 0 * 0 "人名:日本:姓:2:0.00871"
は は は 助詞 9 副助詞 2 * 0 * 0 NIL
大人 おとな 大人 名詞 6 普通名詞 1 * 0 * 0 "代表表記:大人/おとな カテゴリ:人"
に に に 助詞 9 格助詞 1 * 0 * 0 NIL
なり なり なる 動詞 2 * 0 子音動詞ラ行 10 基本連用形 8 "代表表記:成る/なる 自他動詞:他:成す/なす;他:する/する"
@ なり なり なる 動詞 2 * 0 子音動詞ラ行 10 基本連用形 8 "代表表記:鳴る/なる 自他動詞:他:鳴らす/ならす"
たい たい たい 接尾辞 14 形容詞性述語接尾辞 5 イ形容詞アウオ段 18 基本形 2 "代表表記:たい/たい"
EOS
```

In this format Juman++ outputs a sequence of tokens for each input sequence.
Each line corresponds to a single token (morpheme).
A string "EOS" followed by end of line character marks the end of token sequence for the sentence.

There can be lines prefixed with "`@ `".
They correspond to aliasing tokens which are not distinguished by Juman++.
Basically, those are dictionary entries which have identical feature representation.
**Important**: aliasing tokens are outputted in the order they appear in the dictionary.

This format consists of 12 space-separated fields:

* Surface string
* Reading
* Dictionary form
* 8 POS fields with ids
* Semantic features. They are always quoted if they are not empty and NIL is written otherwise.

***Warning***: This format doesn't work well with **half-width** spaces and double quotes (`"`).
We recommend convert them to full-width characters before analysis when using this format.

### Lattice (N-best)

```
# MA-SCORE	rank1:0.769142 rank2:0.659738 rank3:0.596861
-	1	0	0	1	田中	田中/たなか	たなか	田中	名詞	6	人名	5	*	0	*	0	人名:日本:姓:2:0.00871|特徴量スコア:0.648716|言語モデルスコア:-0.0928609|形態素解析スコア:0.555855|ランク:1;3
-	2	0	0	1	田中	田中/たなか	たなか	田中	名詞	6	地名	4	*	0	*	0	自動獲得:Wikipedia|Wikipediaリダイレクト:田中橋　（渡良瀬川）|Wikipedia多義|Wikipedia複合地名|特徴量スコア:0.523702|言語モデルスコア:-0.0928609|形態素解析スコア:0.430841|ランク:2
-	3	1;2	2	2	は	は/は	は	は	助詞	9	副助詞	2	*	0	*	0	特徴量スコア:-0.0945235|言語モデルスコア:-0.0155799|形態素解析スコア:-0.110103|ランク:1;2;3
-	4	3	3	4	大人	大人/おとな	おとな	大人	名詞	6	普通名詞	1	*	0	*	0	カテゴリ:人|特徴量スコア:0.0884067|言語モデルスコア:-0.0971875|形態素解析スコア:-0.00878079|ランク:1;2;3
-	5	4	5	5	に	に/に	に	に	助詞	9	格助詞	1	*	0	*	0	特徴量スコア:-0.204342|言語モデルスコア:0.012112|形態素解析スコア:-0.19223|ランク:1;2;3
-	6	5	6	7	なり	なる/なる	なり	なる	接尾辞	14	動詞性接尾辞	7	子音動詞ラ行	10	基本連用形	8	特徴量スコア:-0.273051|言語モデルスコア:-0.0655761|形態素解析スコア:-0.338628|ランク:3
-	7	5	6	7	なり	成る/なる	なり	なる	動詞	2	*	0	子音動詞ラ行	10	基本連用形	8	自他動詞:他:成す/なす;他:する/する|特徴量スコア:-0.0611007|言語モデルスコア:-0.00404951|形態素解析スコア:-0.0651503|ランク:1;2
-	7	5	6	7	なり	鳴る/なる	なり	なる	動詞	2	*	0	子音動詞ラ行	10	基本連用形	8	自他動詞:他:鳴らす/ならす|特徴量スコア:-0.0611007|言語モデルスコア:-0.00404951|形態素解析スコア:-0.0651503|ランク:1;2
-	8	6;7	8	9	たい	たい/たい	たい	たい	接尾辞	14	形容詞性述語接尾辞	5	イ形容詞アウオ段	18	基本形	2	特徴量スコア:0.286488|言語モデルスコア:-0.0563604|形態素解析スコア:0.230127|ランク:1;2;3
EOS
```

### (Full) Morph

### Segment

## Protobuf-based formats

TODO: document
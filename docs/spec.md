# Analysis Spec

Spec completely defines how Juman++ will do analysis and training.
It can be defined using a C++ DSL or a text file definition.
This file defines the text file definition, but it is 1-to-1 mappable to the
C++ DSL.
See [Jumandic Spec](../src/jumandic/shared/jumandic_spec.cc) for the example of
C++ DSL usage.

It consists of 5 main "parts":

1. Dictionary
1. Features
1. Unknown word handlers
1. Ngram feature templates
1. Training loss definition

**Notice**: Spec is parsed from top to bottom and statements can not refer
to other statements which are lower that themselves.

# Comments

```
# lines starting with "sharp" symbols are comments to the end of the line
```

# Dictionary

This section describes how Juman++ will use raw CSV dictionary.
For each column in CSV dictionary use one `field` statement.
Its general definition is:

```
field [column #] [name] [type] [flags]
```

**Column number** defines 1-based number of column in the CSV file.

**Name** assigns a name for the further references.

**Type** assigns a type for a field.

Currently supported fields are:

* `string`: a string of arbitrary length
* `int`: a 32-bit signed integer
* `string_list`: a list of strings
* `kv_list`: a list of keys and (optional) values

**Flags** modify behavior of fields.

Supported flags are:

* `trie_index`: this field will be used for the dictionary lookup. 
Can be only assigned to string fields.
There **must be** exactly one `trie_index` field in the spec.
* `empty "[value]"`: defines a value which is considered to be empty (either empty string or empty list).
* `storage [fld_ref]`: the string content of the current field will be stored together with the referred field.
* `align [num]`: aligns field content pointers to the 2^*num*. 
The rightmost *num* bits of pointers are not stored.
Can be used to decrease size of the dictionary (sometimes) and increase the analysis speed (often).
No-op for *int* fields.
* `list_sep "[value]"`: the *value* will be used as list separator in the CSV format instead of default " ".
* `kv_sep "[value]"`: the *value* will be used as key-value separator in the CSV format instead of default ":".


## Examples

```
# dictionary lookup field
field 1 surface string trie_index
# aligned to 3-bit boundary
field 2 dicform string align 3 
# store strings with dicform
field 3 reading string storage dicform
# pos usually has a small number of values, so high alignment is recommended
field 4 pos string align 5 
# use = as key-value separator
field 5 features kv_list kv_sep "="  
```

## Remarks

Field names must be unique.


# Features

This section defines additional primitive features 
that can be used for the linear model.
Any *string* or *int* dictionary fields can also be used as features. 
The basic syntax is:

```
feature [name] [=] [definition]
```

The equal sign in the feature definition is optional, but we recommend to use it for clarity.

## Feature definitions

* `placeholder`: defines a placeholder, a special feature that is 0 by default and can be used for
communicating with unknown word handlers.
* `codepoint <offset>`: an input string unicode codepoint value at *offset* from the current word. 
Positive values mean "codepoint after the word".
Negative values mean "codepoint before the word".
For example, `codepoint 1` is the next character after the current word.
`codepoint -5` is the fifth character before the current word.
Offset of 0 is invalid value.
If the actual codepoint index is resolved to be before the first or after the last sentence codepoint
then special values for "before" and "after" are returned.
* `codepoint_type <offset>`: a type of input string unicode codepoint relative to the current word.
Types are a bit set and defined by [this file](../src/util/characters.h). 
Offset as defined same as above, but for *0* the result value becomes a *bitwise or* of all codepoints inside the word.
* `num_codepoints`: number of codepoints in the current word.
* `num_bytes`: number of UTF-8 bytes in the current word.
* `match A with B`: a special conditional feature.


### Conditional Feature

Juman++ supports two types of conditional features:

1. Checking that a value of a dictionary field (or a list of them) is included in a list of predefined values,
yielding 1 or 0
2. Yielding two different lists of features for each case

The basic syntax is 
```
match FIELDS with VALUES
match FIELDS with VALUES then LIST_TRUE else LIST_FALSE
```

`FIELDS` can be either a single field reference or a list of them (`[fld1, fld2]`).
`VALUES` can be either an inline CSV inside quotes (`"value1,value2"`) or a csv file name (`file <filename>`).
The filename will be resolved relative to the spec file.
File syntax is not supported by C++ DSL.

`match [fld1, fld2] with "a,b"` will yield 1 only if for a csv row would have "a" for `fld1` and "b" for the `fld2`
(either a full match in case of string or int; any value of the string list; or any key of key-value list).
Using a file argument allows you to match several values (C++ DSL can use line breaks for the same effect).

The conditional feature can be used for partial lexicalization:
```
feature aux_word = match [pos] with "助詞" then [surface, pos] else [pos]
```

**Notice**: Juman++ computes conditions at the *dictionary build time*, 
so only fields can be referenced at the condition left hand side.
`true` and `false` lists can contain other features as well.

## Examples

```
feature next_cp = codepoint 1
feature length = num_codepoints surface
feature aux_word = match [pos] with "助詞" then [surface, pos] else [pos]
feature lexicalize = match [dicform, pos] with file "lexicalize.csv" then [surface, pos] else [pos]
```

## Remarks

Feature names must be unique and must not use field names.

# Unknown Word Handlers

This section defines unknown word handlers which can add additional nodes to the lattice.
The basic syntax is:

```
unk [name] template row [row #] [:] [<definition>] [feature to <placeholder>] [surface to <field list>]
```

Most of unknown word handlers create a lattice node based on a *template* contained in the dictionary.
Template row number is *1-based*. 
They replace the content of fields specified in the `surface to` clause with the actual surface string.


Unknown word hanlders generally do **not** create a word if there exist one in a dictionary
with the same surface. 


## Character classes

Most of unknown word handlers use character classes defined in [this file](../src/util/characters.h).
Text file spec definition accepts all character classes.
It is possible to use both in upper and lower case.
Also, it is possible to combine different character classes together with `|` character.

Character classes themselves are a bitsets.
We define that two character classes are compatible if a bitwise and operation over
them yield at least one non-zero bit. 

## Placeholders

`feature to <placeholder>` syntax assigns a placeholder feature to the unknown word handler.
Some of handlers will assign values the to assigned placeholders which can be used for analysis. 

## Handler Definitions

* `single <char class>`: creates unknown words containing a single codepoint with the compatible character class.
* `chunking <char class>`: creates unknown words containing a sequence of codepoints of compatible character class.
We create all O(N^2) possible sub-sequences.
**Feature output**: 1 if the dictionary do not contain a word which is a prefix of the current one.
* `onomatopeia <char class>`: creates patterned subsequences of compatible character class.
Possible patterns are ABAB, ABCABC, ABCDABCD.
* `numeric <char class>`: creates unknown words consisting of Japanese numbers;
Literals that are separated with commas by 3 digits like 100,000,000 are also included.
Character class argument is not used.
* `normalize`: creates unknown words using normalization rules.
For example, a surface かーさん will be matched to the かあさん instead.

### Normalization

Normalization does not use pattern.
Instead it modifies a definition of original node (or multiple ones). 
The rules are applied before the dictionary lookup.


* Convert prolongation to the actual character: かーさん → かあさん. 
Prolongation marks are ー or 〜 in all their variations.
We do this only if the previous character was hiragana or katakana.
See [source](../src/util/characters.cc#L168).
* Convert small hiragana/katakata to their normal variations: かぁさん → かあさん
* Remove prolongation: 痛いー → 痛い. 
The condition is one of
    * Previous char is Kanji, Hiragana or Katakana
    * Previous char was deleted    
* Remove small tsu: っ or ッ: 痛いっ → 痛い
* Remove small kana if it is after a compatible character: だめぇ → だめ

**Feature output**: a bitmask corresponding to the applied operations to the placeholder feature.
Bitmask values are defined in [this file](../src/core/analysis/charlattice.h).

## Examples

```
# analysis spec should contain this handler to be able to handle really any input
unk anything template row 1: single family_anything

# katakana unknown word generic definition
unk katakana template row 2: chunking katakana surface to [surface,dicform,reading] feature to [notPrefix]

# normalize feature definition
unk normalize template row 0: normalize surface to [surface,reading] feature to [normalizeState]
```

## Remarks

* Handlers must have unique names.
* Fields which are filled with unknown surfaces **should** be used in ngram feature templates and in training loss,
otherwise their content can be incorrect for duplicated words.
(See remarks on the training loss).   

# Ngram Feature Templates

Juman++ uses features generated using ngram templates for its linear model.
Fields and addditonal features are defined for a node.
Ngrams define which nodes we are going to use.
We support uni,bi and trigrams.

Syntax for ngrams is:
```
ngram [<t0>] # unigram
ngram [<t1>][<t0>] # bigram
ngram [<t2>][<t1>][<t0>] #trigram
```

A node is called *t0* if it starts on a current boundary.
Nodes ending at the same boundary are *t1*.
Previous nodes of *t1*s are t2.

Only string and int typed fields or additional features can be used in ngram patterns.

## Example

```
ngram [pos]
ngram [surface][pos]
ngram [surface,pos][surface,pos][pos]
```

# Training Loss

As the last step we want to define the structure of training loss.
It should include all fields used for training.
The syntax is:

```
train loss <field1> <weight1> [, ...] [flags]
```

The most straightforward thing is to enumerate all fields you are using in the training data with the weight of 1.
Training data should have fields in the same order as the train loss definition.
If the weight is 0, then the field would be expected to be present in the training data,
but it won't be used for loss computation and nodes with the incorrect 0-weighted 
field will be used as gold nodes.

Only string-typed fields can be used for loss.

**Notice**: Only fields which are used in 
* ngram feature pattern
* `match` feature `then` or `else` clause 
* unknown word handler `surface to` clause (read and understand Node Aliasing and Deduplication)
can be used for loss computation.

## Flags

The only supported flag is
```
unk_gold_if <kv_field>[<key>] == <field>
```
It allows Juman++ to treat nodes created by unknown word handlers as gold if
a value of the specified key-value field with the specified key (of an UNK node) is equal 
to the `field` value of a gold node.

## Node Aliasing and Deduplication

**TL;DR**: If a field is used in ngram template, use it in training loss with non-zero weight.

Juman++ distinguishes a node at analysis time by the set of non-0-weighted fields used for loss.
A set of such fields an aliasing set.
If there exist more than one dictionary entry with the same values of the aliasing set nodes,
they will be merged to a single node for the purpose of analysis at dictionary build time. 

The additional features will be computed only for the first dictionary entry of each aliasing set.

If a field is used in features or unknown word handlers at all, 
Juman++ will keep values of such field for each dictionary entry.
Otherwise, only the value of the first dictionary entry is kept.

Usually the aliasing set and a set of fields used in features are the same and there are no problems.
But there can exist a bad situation: a 0-loss-weighted node is used only in unknown word handler.
There will be only the first dictionary value of such field kept for each aliasing set.
If it's possible don't use 0-weighted fields and have all nodes used in ngram features to be used in training loss.


# Dictionary 

Terms:

* Dictionary — csv file
* Field — a column of dictionary csv file defined in the
  dictionary spec
* Spec — a file that describes which csv columns to use and 
  how to interpret them  
* Entry — a line of dictionary

Dictionary (csv file) consits of entries (lines).
Each entry contains several fields (columns).
Each field is described using spec file.


## Dictionary Spec

Spec defines which columns of dictionary will be used.

Let's consider a dictionary like this:

```
である,0,0,0,判定詞,*,デアル列基本形,判定詞,だ,である,*,NIL
らしい,0,0,0,助動詞,*,基本形,イ形容詞イ段,らしい,らしい,*,NIL
```


It can be defined with following spec:

```
# this is a comment
1 SURFACE STRING TRIE_INDEX
5 POS-MAIN STRING
6 POS-SUB STRING
7 POS-CONJFORM STRING
8 POS-CONJTYPE STRING
9 DICFORM STRING #this is comment as well
10 READING STRING
12 FEATURE STRING_LIST
```

Format of spec line is

```
<FIELD_NUMBER> <NAME> <TYPE> <FLAGS>
```

`<FIELD_NUMBER>` is 1-based number of column in csv file.
Each field can be referred only once.

`<NAME>` specifies the name for the field. 
It must not contain spaces or tabs.
Any other character is acceptable. 
`表層形` or `♡` are perfectly acceptable field names.
Each field must have unique name.

`<TYPE>` describes the type of field. 
Currently there are three types supported:
STRING, INT and STRING_LIST.
STRING_LIST individual entries should be separated
by spaces (0x20) and should not contain them.

`<FLAGS>` specifies special flags for fields.
There could be 0 or more flags for each field.
Current list of flags is following.

* `TRIE_INDEX`: (unique) use the field as trie index for the
  analysis.

## Dictionary Storage

Disk storage uses **varint** as the base type.
Basically, it is a mean to store int value
as a variable number of bytes.
[Protocol Buffer Documentation](https://developers.google.com/protocol-buffers/docs/encoding)
has good description of the format
in detail.

Strings are stored in the following form.
```
varint length
uint8* data
```
It can be interperted as length as varint followed
by actual string data as bytes.
Format description will refer to 
this storage format as `string`.

### Format

Each dictionary entry is stored as sequence of varints, 
**one** for each field.
Encoding for the varint is specific for each field type.

Values of `INT` field are stored as varints 
without any indirection.

Values of `STRING` field are stored as offsets
from the beginning from the respective *field domain*.
There is one level of indirection.

Values of `STRING_LIST` field are stored as offset
to the following structure called `list_elements`:
```
varint length
varint* offset_difference
```
Each entry in the second field is not 
a diff-encoded domain pointer.
For example, a sequence of three domain pointers
```
a b c
```
can be restored as
```
a (a+b) (a+b+c)
```
`STRING_LIST` has two levels of indirection.

### Field Domain

Actual values from each `STRING` or `STRING_LIST` field
forms form **data domain** for each field.
Actual field values are deduplicated and 
stored in decreasing frequency order.

Offset from the beginning of domain data is called **domain pointer**.

Domain data is usually has the following format.
```
string* data
```

`STRING_LIST` typed fields have the second domain

```
list_elements* data
```

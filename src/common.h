#pragma once
#ifndef COMMON_H
#define COMMON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <climits>
#include <unordered_map>

#include <boost/functional/hash.hpp>
#include <map>
#include <set>
#include <deque>
#include <queue>
#include <algorithm>
#include <cstdlib>
#include <ios>
#include <iomanip>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "config.h"

typedef std::unordered_map<std::string, double> Umap;

typedef std::vector<double> TopicVector;

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

extern bool MODE_TRAIN;
extern bool WEIGHT_AVERAGED;

int main(int argc, char **argv);

namespace Morph {

extern std::unordered_map<std::string, int> pos_map;
extern std::unordered_map<std::string, int> spos_map;
extern std::unordered_map<std::string, int> katuyou_type_map;
extern std::unordered_map<std::string, int> katuyou_form_map;

extern int utf8_bytes(unsigned char *ucp);
extern size_t utf8_length(const char *ucp);
extern unsigned short utf8_chars(unsigned char *ucp, size_t byte_len);
extern unsigned char *
get_specified_char_pointer(unsigned char *ucp, size_t byte_len,
                           unsigned short specified_char_num);

extern unsigned int check_utf8_char_type(const char *ucp);
extern unsigned int check_utf8_char_type(unsigned char *ucp);

extern unsigned int check_char_family(unsigned char *ucp);
extern unsigned int check_char_family(unsigned int char_type);
extern bool compare_char_type_in_family(unsigned int char_type,
                                        unsigned int family_pattern);
extern unsigned int
check_exceptional_chars_in_figure(const char *cp, unsigned int rest_byte_len);

extern size_t is_suuji(unsigned char *ucp);

#define VERSION "0.96"

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#define INITIAL_MAX_INPUT_LENGTH 1024
#define MAX_RESOLVED_CHAR_NUM 15

#define B_BEST_REDUNDANCY 0.25

#define BOS "<BOS>"
#define EOS "<EOS>"
#define UNK "<UNK>"
#define UNK_CD "<UNK_CD>"
#define UNK_NT "<UNK_NT>"
#define UNK_WIKIPEDIA "<UNK_WIKIPEDIA>"

#define MORPH_NORMAL_NODE 0x0001
#define MORPH_BOS_NODE 0x0002
#define MORPH_EOS_NODE 0x0004
#define MORPH_PSEUDO_NODE 0x0008
#define MORPH_UNK_NODE 0x0010
// NORMAL_NODEでもある
#define MORPH_DEVOICE_NODE 0x0021

// 廃止
// #define MORPH_DUMMY_POS 0xFFFF
#define MORPH_DUMMY_NODE 0xFFFF

#define MORPH_UNK_COST 1000

#define TYPE_SPACE 1
#define TYPE_IDEOGRAPHIC_PUNC 2 // 、。
#define TYPE_KANJI 4
#define TYPE_FIGURE 8
#define TYPE_PERIOD 16     // ．
#define TYPE_MIDDLE_DOT 32 // ・
#define TYPE_COMMA 64      //　，
#define TYPE_ALPH 128
#define TYPE_SYMBOL 256
#define TYPE_KATAKANA 512
#define TYPE_HIRAGANA 1024
#define TYPE_KANJI_FIGURE 2048
#define TYPE_SLASH 4096
#define TYPE_COLON 8192
#define TYPE_ERA 16384                //㍻
#define TYPE_CHOON 0x00008000;        // ー, 〜
#define TYPE_HANKAKU_KANA 0x00010000; // 半角カタカナ

//#define TYPE_FAMILY_FIGURE	14392 // TYPE_FIGURE + TYPE_PERIOD +
//TYPE_MIDDLE_DOT + TYPE_KANJI_FIGURE + TYPE_SLASH + TYPE_COLON
#define TYPE_FAMILY_FIGURE                                                     \
    2168 // TYPE_FIGURE + TYPE_PERIOD + TYPE_COMMA + TYPE_MIDDLE_DOT +
         // TYPE_KANJI_FIGURE  8+16+64+32+2048
#define TYPE_FAMILY_PUNC                                                       \
    114 // TYPE_PERIOD + TYPE_COMMA + TYPE_IDEOGRAPHIC_PUNC + TYPE_MIDDLE_DOT
#define TYPE_FAMILY_ALPH_PUNC                                                  \
    12432 // TYPE_ALPH + TYPE_PERIOD + TYPE_SLASH + TYPE_COLON
#define TYPE_FAMILY_PUNC_SYMBOL                                                \
    12658 // TYPE_PERIOD + TYPE_COMMA + TYPE_IDEOGRAPHIC_PUNC + TYPE_MIDDLE_DOT
          // + TYPE_SYMBOL + TYPE_SLASH + TYPE_COLON

#define TYPE_FAMILY_SPACE 1    // TYPE_SPACE
#define TYPE_FAMILY_SYMBOL 256 // TYPE_SYMBOL
#define TYPE_FAMILY_ALPH 128   // TYPE_ALPH
#define TYPE_FAMILY_KANJI 2052 // TYPE_KANJI + TYPE_KANJI_FIGURE
#define TYPE_FAMILY_OTHERS 0

// for Chinese Treebank
#define SPACE_AND_NONE "_-NONE-"
//#define SPACE " "
#define NONE_POS "-NONE-"
#define UNK_POS "<UNK>"
// #define UNK_POSS "名詞,副詞,感動詞,動詞"
#define UNK_POSS "名詞"
#define UNK_FIGURE_POS "<UNK_CD>"
#define UNK_FIGURE_POSS "名詞"
#define UNK_FIGURE_SPOS "数詞"

#define EXCEPTIONAL_FIGURE_EXPRESSION "分の"
#define EXCEPTIONAL_FIGURE_EXPRESSION_2 "ぶんの"
#define EXCEPTIONAL_FIGURE_EXPRESSION_LENGTH 6
#define EXCEPTIONAL_FIGURE_EXPRESSION_LENGTH_2 9

// split function with split_num
template <class T>
inline int split_string(const std::string &src, const std::string &key,
                        T &result, int split_num) {
    result.clear();
    int len = src.size();
    int i = 0, si = 0, count = 0;

    while (i < len) {
        while (i < len && key.find(src[i]) != std::string::npos) {
            si++;
            i++;
        } // skip beginning spaces
        while (i < len && key.find(src[i]) == std::string::npos)
            i++;                                 // skip contents
        if (split_num && ++count >= split_num) { // reached the specified num
            result.push_back(
                src.substr(si, len - si)); // push the remainder string
            break;
        }
        result.push_back(src.substr(si, i - si));
        si = i;
    }

    return result.size();
}

// split function
template <class T>
inline int split_string(const std::string &src, const std::string &key,
                        T &result) {
    return split_string(src, key, result, 0);
}

// int to string
template <class T> inline std::string int2string(const T i) {
    std::ostringstream o;

    o << i;
    return o.str();
}

inline std::string double2string(double d) {

    char buf[6];
    std::snprintf(buf, 5, "%.4f", d);
    return std::string(buf);
}
}

#endif

#include "common.h"
#include "pos.h"
#include "sentence.h"
#include "feature.h"
#include <sstream>

namespace Morph {

// (almost)constant variables
// どこのクラスかへ移動する
// std::unordered_map<std::string,int> pos_map({//{{{
//    {"*",0}, {"特殊",1}, {"動詞",2}, {"形容詞",3}, {"判定詞",4}, {"助動詞",5},
//    {"名詞",6}, {"指示詞",7},
//    {"副詞",8}, {"助詞",9}, {"接続詞",10}, {"連体詞",11}, {"感動詞",12},
//    {"接頭辞",13}, {"接尾辞",14}, {"未定義語",15}});//}}}
// std::unordered_map<std::string,int> spos_map{//{{{
//    {"*",0},
//    {"句点",1}, {"読点",2}, {"括弧始",3}, {"括弧終",4}, {"記号",5},
//    {"空白",6},
//    {"普通名詞",1}, {"サ変名詞",2}, {"固有名詞",3}, {"地名",4}, {"人名",5},
//    {"組織名",6}, {"数詞",7}, {"形式名詞",8},
//    {"副詞的名詞",9}, {"時相名詞",10}, {"名詞形態指示詞",1},
//    {"連体詞形態指示詞",2}, {"副詞形態指示詞",3}, {"格助詞",1}, {"副助詞",2},
//    {"接続助詞",3}, {"終助詞",4}, {"名詞接頭辞",1}, {"動詞接頭辞",2},
//    {"イ形容詞接頭辞",3}, {"ナ形容詞接頭辞",4}, {"名詞性述語接尾辞",1},
//    {"名詞性名詞接尾辞",2}, {"名詞性名詞助数辞",3}, {"名詞性特殊接尾辞",4},
//    {"形容詞性述語接尾辞",5}, {"形容詞性名詞接尾辞",6},
//    {"動詞性接尾辞",7}, {"その他",1}, {"カタカナ",2},
//    {"アルファベット",3}};//}}}
// std::unordered_map<std::string,int> katuyou_type_map{//{{{
//    {"*",0}, {"母音動詞",1}, {"子音動詞カ行",2}, {"子音動詞カ行促音便形",3},
//    {"子音動詞ガ行",4}, {"子音動詞サ行",5},
//    {"子音動詞タ行",6}, {"子音動詞ナ行",7}, {"子音動詞バ行",8},
//    {"子音動詞マ行",9}, {"子音動詞ラ行",10},
//    {"子音動詞ラ行イ形",11}, {"子音動詞ワ行",12},
//    {"子音動詞ワ行文語音便形",13}, {"カ変動詞",14}, {"カ変動詞来",15},
//    {"サ変動詞",16}, {"ザ変動詞",17}, {"イ形容詞アウオ段",18},
//    {"イ形容詞イ段",19}, {"イ形容詞イ段特殊",20},
//    {"ナ形容詞",21}, {"ナノ形容詞",22}, {"ナ形容詞特殊",23},
//    {"タル形容詞",24}, {"判定詞",25},
//    {"無活用型",26}, {"助動詞ぬ型",27}, {"助動詞だろう型",28},
//    {"助動詞そうだ型",29},
//    {"助動詞く型",30}, {"動詞性接尾辞ます型",31},
//    {"動詞性接尾辞うる型",32}};//}}}
// std::unordered_map<std::string,int>
// katuyou_form_map{{"母音動詞:語幹",1},//{{{
//    {"*:*",0},
//{"母音動詞:基本形",2}, {"母音動詞:未然形",3}, {"母音動詞:意志形",4},
//{"母音動詞:省略意志形",5}, {"母音動詞:命令形",6}, {"母音動詞:基本条件形",7},
//{"母音動詞:基本連用形",8}, {"母音動詞:タ接連用形",9}, {"母音動詞:タ形",10},
//{"母音動詞:タ系推量形",11}, {"母音動詞:タ系省略推量形",12},
//{"母音動詞:タ系条件形",13}, {"母音動詞:タ系連用テ形",14},
//{"母音動詞:タ系連用タリ形",15}, {"母音動詞:タ系連用チャ形",16},
//{"母音動詞:音便条件形",17},
//{"母音動詞:文語命令形",18}, {"子音動詞カ行:語幹",1},
//{"子音動詞カ行:基本形",2}, {"子音動詞カ行:未然形",3},
//{"子音動詞カ行:意志形",4},
//{"子音動詞カ行:省略意志形",5}, {"子音動詞カ行:命令形",6},
//{"子音動詞カ行:基本条件形",7}, {"子音動詞カ行:基本連用形",8},
//{"子音動詞カ行:タ接連用形",9},
//{"子音動詞カ行:タ形",10}, {"子音動詞カ行:タ系推量形",11},
//{"子音動詞カ行:タ系省略推量形",12}, {"子音動詞カ行:タ系条件形",13},
//{"子音動詞カ行:タ系連用テ形",14}, {"子音動詞カ行:タ系連用タリ形",15},
//{"子音動詞カ行:タ系連用チャ形",16}, {"子音動詞カ行:音便条件形",17},
//{"子音動詞カ行促音便形:語幹",1},
//{"子音動詞カ行促音便形:基本形",2}, {"子音動詞カ行促音便形:未然形",3},
//{"子音動詞カ行促音便形:意志形",4}, {"子音動詞カ行促音便形:省略意志形",5},
//{"子音動詞カ行促音便形:命令形",6}, {"子音動詞カ行促音便形:基本条件形",7},
//{"子音動詞カ行促音便形:基本連用形",8}, {"子音動詞カ行促音便形:タ接連用形",9},
//{"子音動詞カ行促音便形:タ形",10}, {"子音動詞カ行促音便形:タ系推量形",11},
//{"子音動詞カ行促音便形:タ系省略推量形",12},
//{"子音動詞カ行促音便形:タ系条件形",13},
//{"子音動詞カ行促音便形:タ系連用テ形",14},
//{"子音動詞カ行促音便形:タ系連用タリ形",15},
//{"子音動詞カ行促音便形:タ系連用チャ形",16},
//{"子音動詞カ行促音便形:音便条件形",17}, {"子音動詞ガ行:語幹",1},
//{"子音動詞ガ行:基本形",2},
//{"子音動詞ガ行:未然形",3}, {"子音動詞ガ行:意志形",4},
//{"子音動詞ガ行:省略意志形",5}, {"子音動詞ガ行:命令形",6},
//{"子音動詞ガ行:基本条件形",7},
//{"子音動詞ガ行:基本連用形",8}, {"子音動詞ガ行:タ接連用形",9},
//{"子音動詞ガ行:タ形",10}, {"子音動詞ガ行:タ系推量形",11},
//{"子音動詞ガ行:タ系省略推量形",12}, {"子音動詞ガ行:タ系条件形",13},
//{"子音動詞ガ行:タ系連用テ形",14}, {"子音動詞ガ行:タ系連用タリ形",15},
//{"子音動詞ガ行:タ系連用チャ形",16},
//{"子音動詞ガ行:音便条件形",17}, {"子音動詞サ行:語幹",1},
//{"子音動詞サ行:基本形",2}, {"子音動詞サ行:未然形",3},
//{"子音動詞サ行:意志形",4},
//{"子音動詞サ行:省略意志形",5}, {"子音動詞サ行:命令形",6},
//{"子音動詞サ行:基本条件形",7}, {"子音動詞サ行:基本連用形",8},
//{"子音動詞サ行:タ接連用形",9},
//{"子音動詞サ行:タ形",10}, {"子音動詞サ行:タ系推量形",11},
//{"子音動詞サ行:タ系省略推量形",12}, {"子音動詞サ行:タ系条件形",13},
//{"子音動詞サ行:タ系連用テ形",14}, {"子音動詞サ行:タ系連用タリ形",15},
//{"子音動詞サ行:タ系連用チャ形",16}, {"子音動詞サ行:音便条件形",17},
//{"子音動詞タ行:語幹",1},
//{"子音動詞タ行:基本形",2}, {"子音動詞タ行:未然形",3},
//{"子音動詞タ行:意志形",4}, {"子音動詞タ行:省略意志形",5},
//{"子音動詞タ行:命令形",6},
//{"子音動詞タ行:基本条件形",7}, {"子音動詞タ行:基本連用形",8},
//{"子音動詞タ行:タ接連用形",9}, {"子音動詞タ行:タ形",10},
//{"子音動詞タ行:タ系推量形",11},
//{"子音動詞タ行:タ系省略推量形",12}, {"子音動詞タ行:タ系条件形",13},
//{"子音動詞タ行:タ系連用テ形",14}, {"子音動詞タ行:タ系連用タリ形",15},
//{"子音動詞タ行:タ系連用チャ形",16}, {"子音動詞タ行:音便条件形",17},
//{"子音動詞ナ行:語幹",1}, {"子音動詞ナ行:基本形",2}, {"子音動詞ナ行:未然形",3},
//{"子音動詞ナ行:意志形",4}, {"子音動詞ナ行:省略意志形",5},
//{"子音動詞ナ行:命令形",6}, {"子音動詞ナ行:基本条件形",7},
//{"子音動詞ナ行:基本連用形",8},
//{"子音動詞ナ行:タ接連用形",9}, {"子音動詞ナ行:タ形",10},
//{"子音動詞ナ行:タ系推量形",11}, {"子音動詞ナ行:タ系省略推量形",12},
//{"子音動詞ナ行:タ系条件形",13}, {"子音動詞ナ行:タ系連用テ形",14},
//{"子音動詞ナ行:タ系連用タリ形",15}, {"子音動詞ナ行:タ系連用チャ形",16},
//{"子音動詞ナ行:音便条件形",17},
//{"子音動詞バ行:語幹",1}, {"子音動詞バ行:基本形",2}, {"子音動詞バ行:未然形",3},
//{"子音動詞バ行:意志形",4}, {"子音動詞バ行:省略意志形",5},
//{"子音動詞バ行:命令形",6}, {"子音動詞バ行:基本条件形",7},
//{"子音動詞バ行:基本連用形",8}, {"子音動詞バ行:タ接連用形",9},
//{"子音動詞バ行:タ形",10},
//{"子音動詞バ行:タ系推量形",11}, {"子音動詞バ行:タ系省略推量形",12},
//{"子音動詞バ行:タ系条件形",13}, {"子音動詞バ行:タ系連用テ形",14},
//{"子音動詞バ行:タ系連用タリ形",15}, {"子音動詞バ行:タ系連用チャ形",16},
//{"子音動詞バ行:音便条件形",17}, {"子音動詞マ行:語幹",1},
//{"子音動詞マ行:基本形",2},
//{"子音動詞マ行:未然形",3}, {"子音動詞マ行:意志形",4},
//{"子音動詞マ行:省略意志形",5}, {"子音動詞マ行:命令形",6},
//{"子音動詞マ行:基本条件形",7},
//{"子音動詞マ行:基本連用形",8}, {"子音動詞マ行:タ接連用形",9},
//{"子音動詞マ行:タ形",10}, {"子音動詞マ行:タ系推量形",11},
//{"子音動詞マ行:タ系省略推量形",12}, {"子音動詞マ行:タ系条件形",13},
//{"子音動詞マ行:タ系連用テ形",14}, {"子音動詞マ行:タ系連用タリ形",15},
//{"子音動詞マ行:タ系連用チャ形",16},
//{"子音動詞マ行:音便条件形",17}, {"子音動詞ラ行:語幹",1},
//{"子音動詞ラ行:基本形",2}, {"子音動詞ラ行:未然形",3},
//{"子音動詞ラ行:意志形",4},
//{"子音動詞ラ行:省略意志形",5}, {"子音動詞ラ行:命令形",6},
//{"子音動詞ラ行:基本条件形",7}, {"子音動詞ラ行:基本連用形",8},
//{"子音動詞ラ行:タ接連用形",9},
//{"子音動詞ラ行:タ形",10}, {"子音動詞ラ行:タ系推量形",11},
//{"子音動詞ラ行:タ系省略推量形",12}, {"子音動詞ラ行:タ系条件形",13},
//{"子音動詞ラ行:タ系連用テ形",14}, {"子音動詞ラ行:タ系連用タリ形",15},
//{"子音動詞ラ行:タ系連用チャ形",16}, {"子音動詞ラ行:音便条件形",17},
//{"子音動詞ラ行イ形:語幹",1},
//{"子音動詞ラ行イ形:基本形",2}, {"子音動詞ラ行イ形:未然形",3},
//{"子音動詞ラ行イ形:意志形",4}, {"子音動詞ラ行イ形:省略意志形",5},
//{"子音動詞ラ行イ形:命令形",6}, {"子音動詞ラ行イ形:基本条件形",7},
//{"子音動詞ラ行イ形:基本連用形",8}, {"子音動詞ラ行イ形:タ接連用形",9},
//{"子音動詞ラ行イ形:タ形",10},
//{"子音動詞ラ行イ形:タ系推量形",11}, {"子音動詞ラ行イ形:タ系省略推量形",12},
//{"子音動詞ラ行イ形:タ系条件形",13}, {"子音動詞ラ行イ形:タ系連用テ形",14},
//{"子音動詞ラ行イ形:タ系連用タリ形",15},
//{"子音動詞ラ行イ形:タ系連用チャ形",16}, {"子音動詞ラ行イ形:音便条件形",17},
//{"子音動詞ワ行:語幹",1}, {"子音動詞ワ行:基本形",2}, {"子音動詞ワ行:未然形",3},
//{"子音動詞ワ行:意志形",4}, {"子音動詞ワ行:省略意志形",5},
//{"子音動詞ワ行:命令形",6},
//{"子音動詞ワ行:基本条件形",7}, {"子音動詞ワ行:基本連用形",8},
//{"子音動詞ワ行:タ接連用形",9}, {"子音動詞ワ行:タ形",10},
//{"子音動詞ワ行:タ系推量形",11},
//{"子音動詞ワ行:タ系省略推量形",12}, {"子音動詞ワ行:タ系条件形",13},
//{"子音動詞ワ行:タ系連用テ形",14}, {"子音動詞ワ行:タ系連用タリ形",15},
//{"子音動詞ワ行:タ系連用チャ形",16}, {"子音動詞ワ行文語音便形:語幹",1},
//{"子音動詞ワ行文語音便形:基本形",2}, {"子音動詞ワ行文語音便形:未然形",3},
//{"子音動詞ワ行文語音便形:意志形",4}, {"子音動詞ワ行文語音便形:省略意志形",5},
//{"子音動詞ワ行文語音便形:命令形",6}, {"子音動詞ワ行文語音便形:基本条件形",7},
//{"子音動詞ワ行文語音便形:基本連用形",8},
//{"子音動詞ワ行文語音便形:タ接連用形",9}, {"子音動詞ワ行文語音便形:タ形",10},
//{"子音動詞ワ行文語音便形:タ系推量形",11},
//{"子音動詞ワ行文語音便形:タ系省略推量形",12},
//{"子音動詞ワ行文語音便形:タ系条件形",13},
//{"子音動詞ワ行文語音便形:タ系連用テ形",14},
//{"子音動詞ワ行文語音便形:タ系連用タリ形",15},
//{"子音動詞ワ行文語音便形:タ系連用チャ形",16},
//{"カ変動詞:語幹",1}, {"カ変動詞:基本形",2}, {"カ変動詞:未然形",3},
//{"カ変動詞:意志形",4}, {"カ変動詞:省略意志形",5},
//{"カ変動詞:命令形",6}, {"カ変動詞:基本条件形",7}, {"カ変動詞:基本連用形",8},
//{"カ変動詞:タ接連用形",9}, {"カ変動詞:タ形",10},
//{"カ変動詞:タ系推量形",11}, {"カ変動詞:タ系省略推量形",12},
//{"カ変動詞:タ系条件形",13}, {"カ変動詞:タ系連用テ形",14},
//{"カ変動詞:タ系連用タリ形",15},
//{"カ変動詞:タ系連用チャ形",16}, {"カ変動詞:音便条件形",17},
//{"カ変動詞来:語幹",1}, {"カ変動詞来:基本形",2}, {"カ変動詞来:未然形",3},
//{"カ変動詞来:意志形",4}, {"カ変動詞来:省略意志形",5}, {"カ変動詞来:命令形",6},
//{"カ変動詞来:基本条件形",7}, {"カ変動詞来:基本連用形",8},
//{"カ変動詞来:タ接連用形",9}, {"カ変動詞来:タ形",10},
//{"カ変動詞来:タ系推量形",11}, {"カ変動詞来:タ系省略推量形",12},
//{"カ変動詞来:タ系条件形",13},
//{"カ変動詞来:タ系連用テ形",14}, {"カ変動詞来:タ系連用タリ形",15},
//{"カ変動詞来:タ系連用チャ形",16}, {"カ変動詞来:音便条件形",17},
//{"サ変動詞:語幹",1},
//{"サ変動詞:基本形",2}, {"サ変動詞:未然形",3}, {"サ変動詞:意志形",4},
//{"サ変動詞:省略意志形",5}, {"サ変動詞:命令形",6},
//{"サ変動詞:基本条件形",7}, {"サ変動詞:基本連用形",8},
//{"サ変動詞:タ接連用形",9}, {"サ変動詞:タ形",10}, {"サ変動詞:タ系推量形",11},
//{"サ変動詞:タ系省略推量形",12}, {"サ変動詞:タ系条件形",13},
//{"サ変動詞:タ系連用テ形",14}, {"サ変動詞:タ系連用タリ形",15},
//{"サ変動詞:タ系連用チャ形",16}, {"サ変動詞:音便条件形",17},
//{"サ変動詞:文語基本形",18}, {"サ変動詞:文語未然形",19},
//{"サ変動詞:文語命令形",20},
//{"ザ変動詞:語幹",1}, {"ザ変動詞:基本形",2}, {"ザ変動詞:未然形",3},
//{"ザ変動詞:意志形",4}, {"ザ変動詞:省略意志形",5},
//{"ザ変動詞:命令形",6}, {"ザ変動詞:基本条件形",7}, {"ザ変動詞:基本連用形",8},
//{"ザ変動詞:タ接連用形",9}, {"ザ変動詞:タ形",10},
//{"ザ変動詞:タ系推量形",11}, {"ザ変動詞:タ系省略推量形",12},
//{"ザ変動詞:タ系条件形",13}, {"ザ変動詞:タ系連用テ形",14},
//{"ザ変動詞:タ系連用タリ形",15},
//{"ザ変動詞:タ系連用チャ形",16}, {"ザ変動詞:音便条件形",17},
//{"ザ変動詞:文語基本形",18}, {"ザ変動詞:文語未然形",19},
//{"ザ変動詞:文語命令形",20},
//{"イ形容詞アウオ段:語幹",1}, {"イ形容詞アウオ段:基本形",2},
//{"イ形容詞アウオ段:命令形",3}, {"イ形容詞アウオ段:基本推量形",4},
//{"イ形容詞アウオ段:基本省略推量形",5}, {"イ形容詞アウオ段:基本条件形",6},
//{"イ形容詞アウオ段:基本連用形",7}, {"イ形容詞アウオ段:タ形",8},
//{"イ形容詞アウオ段:タ系推量形",9},
//{"イ形容詞アウオ段:タ系省略推量形",10}, {"イ形容詞アウオ段:タ系条件形",11},
//{"イ形容詞アウオ段:タ系連用テ形",12}, {"イ形容詞アウオ段:タ系連用タリ形",13},
//{"イ形容詞アウオ段:タ系連用チャ形",14},
//{"イ形容詞アウオ段:タ系連用チャ形２",15}, {"イ形容詞アウオ段:音便条件形",16},
//{"イ形容詞アウオ段:音便条件形２",17}, {"イ形容詞アウオ段:文語基本形",18},
//{"イ形容詞アウオ段:文語未然形",19},
//{"イ形容詞アウオ段:文語連用形",20}, {"イ形容詞アウオ段:文語連体形",21},
//{"イ形容詞アウオ段:文語命令形",22}, {"イ形容詞アウオ段:エ基本形",23},
//{"イ形容詞イ段:語幹",1}, {"イ形容詞イ段:基本形",2}, {"イ形容詞イ段:命令形",3},
//{"イ形容詞イ段:基本推量形",4}, {"イ形容詞イ段:基本省略推量形",5},
//{"イ形容詞イ段:基本条件形",6}, {"イ形容詞イ段:基本連用形",7},
//{"イ形容詞イ段:タ形",8}, {"イ形容詞イ段:タ系推量形",9},
//{"イ形容詞イ段:タ系省略推量形",10}, {"イ形容詞イ段:タ系条件形",11},
//{"イ形容詞イ段:タ系連用テ形",12}, {"イ形容詞イ段:タ系連用タリ形",13},
//{"イ形容詞イ段:タ系連用チャ形",14},
//{"イ形容詞イ段:タ系連用チャ形２",15}, {"イ形容詞イ段:音便条件形",16},
//{"イ形容詞イ段:音便条件形２",17}, {"イ形容詞イ段:文語基本形",18},
//{"イ形容詞イ段:文語未然形",19}, {"イ形容詞イ段:文語連用形",20},
//{"イ形容詞イ段:文語連体形",21}, {"イ形容詞イ段:文語命令形",22},
//{"イ形容詞イ段:エ基本形",23},
//{"イ形容詞イ段特殊:語幹",1}, {"イ形容詞イ段特殊:基本形",2},
//{"イ形容詞イ段特殊:命令形",3}, {"イ形容詞イ段特殊:基本推量形",4},
//{"イ形容詞イ段特殊:基本省略推量形",5}, {"イ形容詞イ段特殊:基本条件形",6},
//{"イ形容詞イ段特殊:基本連用形",7}, {"イ形容詞イ段特殊:タ形",8},
//{"イ形容詞イ段特殊:タ系推量形",9},
//{"イ形容詞イ段特殊:タ系省略推量形",10}, {"イ形容詞イ段特殊:タ系条件形",11},
//{"イ形容詞イ段特殊:タ系連用テ形",12}, {"イ形容詞イ段特殊:タ系連用タリ形",13},
//{"イ形容詞イ段特殊:タ系連用チャ形",14},
//{"イ形容詞イ段特殊:タ系連用チャ形２",15}, {"イ形容詞イ段特殊:音便条件形",16},
//{"イ形容詞イ段特殊:音便条件形２",17}, {"イ形容詞イ段特殊:文語基本形",18},
//{"イ形容詞イ段特殊:文語未然形",19},
//{"イ形容詞イ段特殊:文語連用形",20}, {"イ形容詞イ段特殊:文語連体形",21},
//{"イ形容詞イ段特殊:文語命令形",22}, {"イ形容詞イ段特殊:エ基本形",23},
//{"ナ形容詞:語幹",1}, {"ナ形容詞:基本形",2}, {"ナ形容詞:ダ列基本連体形",3},
//{"ナ形容詞:ダ列基本推量形",4}, {"ナ形容詞:ダ列基本省略推量形",5},
//{"ナ形容詞:ダ列基本条件形",6}, {"ナ形容詞:ダ列基本連用形",7},
//{"ナ形容詞:ダ列タ形",8}, {"ナ形容詞:ダ列タ系推量形",9},
//{"ナ形容詞:ダ列タ系省略推量形",10}, {"ナ形容詞:ダ列タ系条件形",11},
//{"ナ形容詞:ダ列タ系連用テ形",12}, {"ナ形容詞:ダ列タ系連用タリ形",13},
//{"ナ形容詞:ダ列タ系連用ジャ形",14},
//{"ナ形容詞:ダ列文語連体形",15}, {"ナ形容詞:ダ列文語条件形",16},
//{"ナ形容詞:デアル列基本形",17}, {"ナ形容詞:デアル列命令形",18},
//{"ナ形容詞:デアル列基本推量形",19}, {"ナ形容詞:デアル列基本省略推量形",20},
//{"ナ形容詞:デアル列基本条件形",21}, {"ナ形容詞:デアル列基本連用形",22},
//{"ナ形容詞:デアル列タ形",23}, {"ナ形容詞:デアル列タ系推量形",24},
//{"ナ形容詞:デアル列タ系省略推量形",25}, {"ナ形容詞:デアル列タ系条件形",26},
//{"ナ形容詞:デアル列タ系連用テ形",27}, {"ナ形容詞:デアル列タ系連用タリ形",28},
//{"ナ形容詞:デス列基本形",29}, {"ナ形容詞:デス列基本推量形",30},
//{"ナ形容詞:デス列基本省略推量形",31},
//{"ナ形容詞:デス列タ形",32}, {"ナ形容詞:デス列タ系推量形",33},
//{"ナ形容詞:デス列タ系省略推量形",34}, {"ナ形容詞:デス列タ系条件形",35},
//{"ナ形容詞:デス列タ系連用テ形",36}, {"ナ形容詞:デス列タ系連用タリ形",37},
//{"ナ形容詞:ヤ列基本形",38}, {"ナ形容詞:ヤ列基本推量形",39},
//{"ナ形容詞:ヤ列基本省略推量形",40}, {"ナ形容詞:ヤ列タ形",41},
//{"ナ形容詞:ヤ列タ系推量形",42}, {"ナ形容詞:ヤ列タ系省略推量形",43},
//{"ナ形容詞:ヤ列タ系条件形",44},
//{"ナ形容詞:ヤ列タ系連用タリ形",45}, {"ナノ形容詞:語幹",1},
//{"ナノ形容詞:基本形",2}, {"ナノ形容詞:ダ列基本連体形",3},
//{"ナノ形容詞:ダ列特殊連体形",4},
//{"ナノ形容詞:ダ列基本推量形",5}, {"ナノ形容詞:ダ列基本省略推量形",6},
//{"ナノ形容詞:ダ列基本条件形",7}, {"ナノ形容詞:ダ列基本連用形",8},
//{"ナノ形容詞:ダ列タ形",9}, {"ナノ形容詞:ダ列タ系推量形",10},
//{"ナノ形容詞:ダ列タ系省略推量形",11}, {"ナノ形容詞:ダ列タ系条件形",12},
//{"ナノ形容詞:ダ列タ系連用テ形",13}, {"ナノ形容詞:ダ列タ系連用タリ形",14},
//{"ナノ形容詞:ダ列タ系連用ジャ形",15}, {"ナノ形容詞:ダ列文語連体形",16},
//{"ナノ形容詞:ダ列文語条件形",17},
//{"ナノ形容詞:デアル列基本形",18}, {"ナノ形容詞:デアル列命令形",19},
//{"ナノ形容詞:デアル列基本推量形",20},
//{"ナノ形容詞:デアル列基本省略推量形",21},
//{"ナノ形容詞:デアル列基本条件形",22}, {"ナノ形容詞:デアル列基本連用形",23},
//{"ナノ形容詞:デアル列タ形",24}, {"ナノ形容詞:デアル列タ系推量形",25},
//{"ナノ形容詞:デアル列タ系省略推量形",26},
//{"ナノ形容詞:デアル列タ系条件形",27}, {"ナノ形容詞:デアル列タ系連用テ形",28},
//{"ナノ形容詞:デアル列タ系連用タリ形",29}, {"ナノ形容詞:デス列基本形",30},
//{"ナノ形容詞:デス列基本推量形",31}, {"ナノ形容詞:デス列基本省略推量形",32},
//{"ナノ形容詞:デス列タ形",33}, {"ナノ形容詞:デス列タ系推量形",34},
//{"ナノ形容詞:デス列タ系省略推量形",35}, {"ナノ形容詞:デス列タ系条件形",36},
//{"ナノ形容詞:デス列タ系連用テ形",37}, {"ナノ形容詞:デス列タ系連用タリ形",38},
//{"ナノ形容詞:ヤ列基本形",39}, {"ナノ形容詞:ヤ列基本推量形",40},
//{"ナノ形容詞:ヤ列基本省略推量形",41}, {"ナノ形容詞:ヤ列タ形",42},
//{"ナノ形容詞:ヤ列タ系推量形",43}, {"ナノ形容詞:ヤ列タ系省略推量形",44},
//{"ナノ形容詞:ヤ列タ系条件形",45}, {"ナノ形容詞:ヤ列タ系連用タリ形",46},
//{"ナ形容詞特殊:語幹",1},
//{"ナ形容詞特殊:基本形",2}, {"ナ形容詞特殊:ダ列基本連体形",3},
//{"ナ形容詞特殊:ダ列特殊連体形",4}, {"ナ形容詞特殊:ダ列基本推量形",5},
//{"ナ形容詞特殊:ダ列基本省略推量形",6}, {"ナ形容詞特殊:ダ列基本条件形",7},
//{"ナ形容詞特殊:ダ列基本連用形",8}, {"ナ形容詞特殊:ダ列特殊連用形",9},
//{"ナ形容詞特殊:ダ列タ形",10}, {"ナ形容詞特殊:ダ列タ系推量形",11},
//{"ナ形容詞特殊:ダ列タ系省略推量形",12}, {"ナ形容詞特殊:ダ列タ系条件形",13},
//{"ナ形容詞特殊:ダ列タ系連用テ形",14}, {"ナ形容詞特殊:ダ列タ系連用タリ形",15},
//{"ナ形容詞特殊:ダ列タ系連用ジャ形",16}, {"ナ形容詞特殊:ダ列文語連体形",17},
//{"ナ形容詞特殊:ダ列文語条件形",18}, {"ナ形容詞特殊:デアル列基本形",19},
//{"ナ形容詞特殊:デアル列命令形",20}, {"ナ形容詞特殊:デアル列基本推量形",21},
//{"ナ形容詞特殊:デアル列基本省略推量形",22},
//{"ナ形容詞特殊:デアル列基本条件形",23},
//{"ナ形容詞特殊:デアル列基本連用形",24}, {"ナ形容詞特殊:デアル列タ形",25},
//{"ナ形容詞特殊:デアル列タ系推量形",26},
//{"ナ形容詞特殊:デアル列タ系省略推量形",27},
//{"ナ形容詞特殊:デアル列タ系条件形",28},
//{"ナ形容詞特殊:デアル列タ系連用テ形",29},
//{"ナ形容詞特殊:デアル列タ系連用タリ形",30}, {"ナ形容詞特殊:デス列基本形",31},
//{"ナ形容詞特殊:デス列基本推量形",32},
//{"ナ形容詞特殊:デス列基本省略推量形",33}, {"ナ形容詞特殊:デス列タ形",34},
//{"ナ形容詞特殊:デス列タ系推量形",35},
//{"ナ形容詞特殊:デス列タ系省略推量形",36},
//{"ナ形容詞特殊:デス列タ系条件形",37}, {"ナ形容詞特殊:デス列タ系連用テ形",38},
//{"ナ形容詞特殊:デス列タ系連用タリ形",39}, {"ナ形容詞特殊:ヤ列基本形",40},
//{"ナ形容詞特殊:ヤ列基本推量形",41}, {"ナ形容詞特殊:ヤ列基本省略推量形",42},
//{"ナ形容詞特殊:ヤ列タ形",43}, {"ナ形容詞特殊:ヤ列タ系推量形",44},
//{"ナ形容詞特殊:ヤ列タ系省略推量形",45}, {"ナ形容詞特殊:ヤ列タ系条件形",46},
//{"ナ形容詞特殊:ヤ列タ系連用タリ形",47}, {"タル形容詞:語幹",1},
//{"タル形容詞:基本形",2}, {"タル形容詞:基本連用形",3},
//{"判定詞:語幹",1}, {"判定詞:基本形",2}, {"判定詞:ダ列基本連体形",3},
//{"判定詞:ダ列特殊連体形",4}, {"判定詞:ダ列基本推量形",5},
//{"判定詞:ダ列基本省略推量形",6}, {"判定詞:ダ列基本条件形",7},
//{"判定詞:ダ列タ形",8}, {"判定詞:ダ列タ系推量形",9},
//{"判定詞:ダ列タ系省略推量形",10},
//{"判定詞:ダ列タ系条件形",11}, {"判定詞:ダ列タ系連用テ形",12},
//{"判定詞:ダ列タ系連用タリ形",13}, {"判定詞:ダ列タ系連用ジャ形",14},
//{"判定詞:デアル列基本形",15}, {"判定詞:デアル列命令形",16},
//{"判定詞:デアル列基本推量形",17}, {"判定詞:デアル列基本省略推量形",18},
//{"判定詞:デアル列基本条件形",19},
//{"判定詞:デアル列基本連用形",20}, {"判定詞:デアル列タ形",21},
//{"判定詞:デアル列タ系推量形",22}, {"判定詞:デアル列タ系省略推量形",23},
//{"判定詞:デアル列タ系条件形",24}, {"判定詞:デアル列タ系連用テ形",25},
//{"判定詞:デアル列タ系連用タリ形",26}, {"判定詞:デス列基本形",27},
//{"判定詞:デス列基本推量形",28},
//{"判定詞:デス列基本省略推量形",29}, {"判定詞:デス列タ形",30},
//{"判定詞:デス列タ系推量形",31}, {"判定詞:デス列タ系省略推量形",32},
//{"判定詞:デス列タ系条件形",33}, {"判定詞:デス列タ系連用テ形",34},
//{"判定詞:デス列タ系連用タリ形",35}, {"無活用型:語幹",1},
//{"無活用型:基本形",2},
//{"助動詞ぬ型:語幹",1}, {"助動詞ぬ型:基本形",2}, {"助動詞ぬ型:基本条件形",3},
//{"助動詞ぬ型:基本連用形",4}, {"助動詞ぬ型:基本推量形",5},
//{"助動詞ぬ型:基本省略推量形",6}, {"助動詞ぬ型:タ形",7},
//{"助動詞ぬ型:タ系条件形",8}, {"助動詞ぬ型:タ系連用テ形",9},
//{"助動詞ぬ型:タ系推量形",10},
//{"助動詞ぬ型:タ系省略推量形",11}, {"助動詞ぬ型:音便基本形",12},
//{"助動詞ぬ型:音便推量形",13}, {"助動詞ぬ型:音便省略推量形",14},
//{"助動詞ぬ型:文語連体形",15}, {"助動詞ぬ型:文語条件形",16},
//{"助動詞ぬ型:文語音便条件形",17}, {"助動詞だろう型:語幹",1},
//{"助動詞だろう型:基本形",2},
//{"助動詞だろう型:ダ列基本省略推量形",3}, {"助動詞だろう型:ダ列基本条件形",4},
//{"助動詞だろう型:デアル列基本推量形",5},
//{"助動詞だろう型:デアル列基本省略推量形",6},
//{"助動詞だろう型:デス列基本推量形",7},
//{"助動詞だろう型:デス列基本省略推量形",8},
//{"助動詞だろう型:ヤ列基本推量形",9}, {"助動詞だろう型:ヤ列基本省略推量形",10},
//{"助動詞そうだ型:語幹",1}, {"助動詞そうだ型:基本形",2},
//{"助動詞そうだ型:ダ列タ系連用テ形",3}, {"助動詞そうだ型:デアル列基本形",4},
//{"助動詞そうだ型:デス列基本形",5}, {"助動詞く型:語幹",1},
//{"助動詞く型:基本形",2}, {"助動詞く型:基本連用形",3},
//{"助動詞く型:文語連体形",4}, {"動詞性接尾辞ます型:語幹",1},
//{"動詞性接尾辞ます型:基本形",2}, {"動詞性接尾辞ます型:未然形",3},
//{"動詞性接尾辞ます型:意志形",4}, {"動詞性接尾辞ます型:省略意志形",5},
//{"動詞性接尾辞ます型:命令形",6}, {"動詞性接尾辞ます型:タ形",7},
//{"動詞性接尾辞ます型:タ系条件形",8}, {"動詞性接尾辞ます型:タ系連用テ形",9},
//{"動詞性接尾辞ます型:タ系連用タリ形",10}, {"動詞性接尾辞うる型:語幹",1},
//{"動詞性接尾辞うる型:基本形",2}, {"動詞性接尾辞うる型:基本条件形",3}};//}}}

std::string BOS_STRING = BOS;
std::string EOS_STRING = EOS;

// for test sentence
Sentence::Sentence(std::vector<Node *> *in_begin_node_list,
                   std::vector<Node *> *in_end_node_list,
                   std::string &in_sentence, Dic *in_dic,
                   FeatureTemplateSet *in_ftmpl, Parameter *in_param) { //{{{
    sentence_c_str = in_sentence.c_str();
    length = strlen(sentence_c_str);
    init(length, in_begin_node_list, in_end_node_list, in_dic, in_ftmpl,
         in_param);
} //}}}

// for gold sentence
Sentence::Sentence(size_t max_byte_length,
                   std::vector<Node *> *in_begin_node_list,
                   std::vector<Node *> *in_end_node_list, Dic *in_dic,
                   FeatureTemplateSet *in_ftmpl, Parameter *in_param) { //{{{
    length = 0;
    init(max_byte_length, in_begin_node_list, in_end_node_list, in_dic,
         in_ftmpl, in_param);
} //}}}

void Sentence::init(size_t max_byte_length,
                    std::vector<Node *> *in_begin_node_list,
                    std::vector<Node *> *in_end_node_list, Dic *in_dic,
                    FeatureTemplateSet *in_ftmpl, Parameter *in_param) { //{{{
    param = in_param;
    dic = in_dic;
    ftmpl = in_ftmpl;
    word_num = 0;
    feature = NULL;
    reached_pos = 0;
    reached_pos_of_pseudo_nodes = 0;

    begin_node_list = in_begin_node_list;
    end_node_list = in_end_node_list;
    output_ambiguous_word = in_param->output_ambiguous_word;

    if (begin_node_list->capacity() <= max_byte_length) {
        begin_node_list->reserve(max_byte_length + 1);
        end_node_list->reserve(max_byte_length + 1);
    }
    // コンストラクタで初期化されるから不要
    // memset(&((*begin_node_list)[0]), 0, sizeof((*begin_node_list)[0]) *
    // (max_byte_length + 1));
    // memset(&((*end_node_list)[0]), 0, sizeof((*end_node_list)[0]) *
    // (max_byte_length + 1));

    begin_node_list->clear();
    begin_node_list->resize(max_byte_length + 1, nullptr);
    end_node_list->clear();
    end_node_list->resize(max_byte_length + 1, nullptr);
    (*end_node_list)[0] = get_bos_node(); // Begin Of Sentence
} //}}}

Sentence::~Sentence() { //{{{
    if (feature)
        delete feature;
    clear_nodes();
} //}}}

void Sentence::clear_nodes() { //{{{
    if (end_node_list && end_node_list->size() > 0 && (*end_node_list)[0]) {
        delete (*end_node_list)[0]; // delete BOS
    } else {
        // std::cerr << "skipped: " << this->sentence << " clear_end_nodes" <<
        // std::endl;
    }
    if (begin_node_list && begin_node_list->size() > 0) {
        for (unsigned int pos = 0; pos <= length; pos++) {
            Node *tmp_node = (*begin_node_list)[pos];
            while (tmp_node) {
                // std::cerr << *tmp_node->string_for_print << std::endl;
                Node *next_node = tmp_node->bnext;
                delete tmp_node;
                tmp_node = next_node;
            }
        }
    } else {
        // std::cerr << "skipped: " << this->sentence << " clear_begin_nodes" <<
        // std::endl;
    }

    // memset のせいでメンバにあるオブジェクトが死ぬ
    if (begin_node_list)
        begin_node_list->clear();
    if (end_node_list)
        end_node_list->clear();
} //}}}

bool Sentence::add_one_word(std::string &word) { //{{{
    word_num++;
    length += strlen(word.c_str());
    sentence += word;
    return true;
} //}}}

void Sentence::feature_print() { //{{{
    feature->print();
} //}}}

// make unknown word candidates of specified length if it's not found in dic
Node *Sentence::make_unk_pseudo_node_list_by_dic_check(
    const char *start_str, unsigned int pos, Node *r_node,
    unsigned int specified_char_num) { //{{{
    bool find_this_length = false;
    Node *tmp_node = r_node;
    while (tmp_node) {
        if (tmp_node->char_num == specified_char_num) {
            find_this_length = true;
            break;
        }
        tmp_node = tmp_node->bnext;
    }

    if (!find_this_length) { // if a node of this length is not found in dic
        Node *result_node = dic->make_unk_pseudo_node_list(
            start_str + pos, specified_char_num, specified_char_num);
        if (result_node) {
            if (r_node) {
                tmp_node = result_node;
                while (tmp_node->bnext)
                    tmp_node = tmp_node->bnext;
                tmp_node->bnext = r_node;
            }
            return result_node;
        }
    }
    return r_node;
} //}}}

Node *Sentence::make_unk_pseudo_node_list_from_previous_position(
    const char *start_str, unsigned int previous_pos) { //{{{
    if ((*end_node_list)[previous_pos] != NULL) {
        Node **node_p = &((*begin_node_list)[previous_pos]);
        while (*node_p) {
            node_p = &((*node_p)->bnext);
        }
        *node_p = dic->make_unk_pseudo_node_list(start_str + previous_pos, 2,
                                                 param->unk_max_length);
        find_reached_pos(previous_pos, *node_p);
        set_end_node_list(previous_pos, *node_p);
        return *node_p;
    } else {
        return NULL;
    }
} //}}}

Node *Sentence::make_unk_pseudo_node_list_from_some_positions(
    const char *start_str, unsigned int pos, unsigned int previous_pos) { //{{{
    Node *node;
    node = dic->make_unk_pseudo_node_list(start_str + pos, 1,
                                          param->unk_max_length);
    set_begin_node_list(pos, node);
    find_reached_pos(pos, node);

    // make unknown words from the prevous position
    // if (pos > 0)
    //    make_unk_pseudo_node_list_from_previous_position(start_str,
    //    previous_pos);

    return node;
} //}}}

Node *Sentence::lookup_and_make_special_pseudo_nodes(const char *start_str,
                                                     unsigned int pos) { //{{{
    return lookup_and_make_special_pseudo_nodes(start_str, pos, 0, NULL);
} //}}}

Node *Sentence::lookup_and_make_special_pseudo_nodes(
    const char *start_str, unsigned int specified_length,
    std::string *specified_pos) { //{{{
    return lookup_and_make_special_pseudo_nodes(start_str, 0, specified_length,
                                                specified_pos);
} //}}}

Node *Sentence::lookup_and_make_special_pseudo_nodes(
    const char *start_str, unsigned int pos, unsigned int specified_length,
    std::string *specified_pos) { //{{{
    Node *result_node = NULL;
    Node *kanji_result_node = NULL;

    // まず探す
    // Node *dic_node = dic->lookup_lattice(start_str + pos, specified_length,
    // specified_pos); // look up a dictionary with common prefix search
    Node *dic_node = dic->lookup(
        start_str + pos, specified_length,
        specified_pos); // look up a dictionary with common prefix search

    // 同じ品詞で同じ長さなら使わない

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
    if (specified_length || pos >= reached_pos_of_pseudo_nodes) {
        // 数詞処理
        // 同じ文字種が続く範囲を一語として入れてくれる
        result_node = dic->make_specified_pseudo_node(
            start_str + pos, specified_length, specified_pos,
            &(param->unk_figure_pos), TYPE_FAMILY_FIGURE);
        // make alphabet nodes
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_FAMILY_ALPH_PUNC);
        }

        // カタカナ
        // TODO:適当に切れるところまでで未定義語を作る
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_KATAKANA, dic_node);
        }

        if (!specified_length && result_node) // only prediction
            find_reached_pos_of_pseudo_nodes(pos, result_node);
    }

    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext) //末尾にシーク
            tmp_node = tmp_node->bnext;

        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            auto next_result = tmp_result_node->bnext;
            //            //
            //            使う場合はコピーして元はdelete，使わない場合は単にdelete
            //            if(check_dict_match(tmp_result_node, dic_node)){
            //                tmp_node->bnext = new Node(*tmp_result_node);
            //                tmp_node = tmp_node->bnext;
            //            }
            //            delete tmp_result_node;
            // 使う場合はコピー，使わない場合はdelete
            if (check_dict_match(tmp_result_node, dic_node)) {
                tmp_node->bnext = tmp_result_node;
                tmp_node = tmp_node->bnext;
            } else {
                delete tmp_result_node;
            }
            tmp_result_node = next_result;
        }
        tmp_node->bnext = nullptr;
        result_node = dic_node;
    }

    return result_node;
} //}}}

// GOLD 用の詳細指定版，ほぼ同内容だが，，，
Node *Sentence::lookup_and_make_special_pseudo_nodes(
    const char *start_str, unsigned int specified_length,
    const std::vector<std::string> &spec) { //{{{
    Node *result_node = NULL;
    Node *kanji_result_node = NULL;
    int pos = 0;
    // spec
    // 0: reading, 1:base, 2:pos, 3:spos, 4:type, 5:form
    auto specified_pos_string = spec[2]; // pos
    auto specified_pos = &specified_pos_string;

    // まず探す
    Node *dic_node =
        dic->lookup(start_str, specified_length,
                    spec); // look up a dictionary with common prefix search

    // 同じ品詞で同じ長さなら使わない

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
    if (specified_length || pos >= reached_pos_of_pseudo_nodes) {
        // 数詞処理
        // make figure nodes
        // 同じ文字種が続く範囲を一語とする
        result_node = dic->make_specified_pseudo_node(
            start_str + pos, specified_length, specified_pos,
            &(param->unk_figure_pos), TYPE_FAMILY_FIGURE);
        // 長さが指定されたものに合っていれば，そのまま返す //
        // TYPE_FIGUREに含まれる文字でもでも"数"や"，"は他の品詞になる可能性がある
        // if (specified_length && result_node) return result_node;

        // make alphabet nodes
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_FAMILY_ALPH_PUNC);
            // アルファベットは辞書に載っている可能性がある
            // if (specified_length && result_node) return result_node;
        }

        // カタカナ
        // TODO:適当に切れるところまでで未定義語を作る
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_KATAKANA, dic_node);
        }

        if (!specified_length && result_node) // only prediction
            find_reached_pos_of_pseudo_nodes(pos, result_node);
    }

    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext) //末尾にシーク
            tmp_node = tmp_node->bnext;

        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            auto next_result = tmp_result_node->bnext;
            if (check_dict_match(tmp_result_node, dic_node)) {
                tmp_node->bnext = tmp_result_node;
                tmp_node = tmp_node->bnext;
            } else {
                delete tmp_result_node;
            }
            tmp_result_node = next_result;
        }
        tmp_node->bnext = nullptr;
        result_node = dic_node;
    }

    return result_node;
} //}}}

// TODO::本来はNode あたりに置く
bool Sentence::check_dict_match(Node *tmp_node, Node *dic_node) { //{{{

    //辞書に一致する長さと品詞の形態素があればなければtrue, あればfalse
    if (!tmp_node)
        return false;

    Node *tmp_dic_node = dic_node;
    bool matched = false;
    while (tmp_dic_node) {
        if (tmp_node->length == tmp_dic_node->length && // length が一致
            tmp_node->posid == tmp_dic_node->posid) {   // pos が一致
            matched = true;
            break;
        }
        tmp_dic_node = tmp_dic_node->bnext;
    }

    return !matched;
} //}}}

Node *Sentence::lookup_and_make_special_pseudo_nodes_lattice(
    CharLattice &cl, const char *start_str, unsigned int char_num,
    unsigned int pos, unsigned int specified_length,
    std::string *specified_pos) { //{{{
    Node *result_node = NULL;
    Node *kanji_result_node = NULL;

    // まず探す
    auto lattice_result = cl.da_search_from_position(
        dic->darts, char_num); // こっちは何文字目かが必要
    // 以下は何バイト目かが必要
    Node *dic_node = dic->lookup_lattice(
        lattice_result, start_str + pos, specified_length,
        specified_pos); // look up a dictionary with common prefix search
    // Node *dic_node = dic->lookup(start_str + pos, specified_length,
    // specified_pos); // look up a dictionary with common prefix search

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
    if (specified_length || pos >= reached_pos_of_pseudo_nodes) {
        // 数詞処理
        // make figure nodes
        // 同じ文字種が続く範囲を一語として入れてくれる
        result_node = dic->make_specified_pseudo_node(
            start_str + pos, specified_length, specified_pos,
            &(param->unk_figure_pos), TYPE_FAMILY_FIGURE);
        // 長さが指定されたものに合っていれば，そのまま返す
        // if (specified_length && result_node) return result_node;

        // make alphabet nodes
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_FAMILY_ALPH_PUNC);
            // if (specified_length && result_node) return result_node;
        }

        // 漢字
        // 辞書にあれば、辞書にない品詞だけを追加する
        //        kanji_result_node =
        //        dic->make_specified_pseudo_node_by_dic_check(start_str + pos,
        //                specified_length, specified_pos, &(param->unk_pos),
        //                TYPE_KANJI|TYPE_KANJI_FIGURE, dic_node);
        //        if (result_node && kanji_result_node) {
        //            //数詞の場合はひとつしか返ってこないことが確定しているので、次のノードに追加すれば大丈夫
        //            result_node->bnext = kanji_result_node;
        //        }else if(!result_node){
        //            result_node = kanji_result_node;
        //        }

        // カタカナ
        // TODO:適当に切れるところまでで未定義語を作る
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_KATAKANA, dic_node);
        }

        // ひらがな
        //		if (!result_node) {
        //			result_node =
        // dic->make_specified_pseudo_node(start_str + pos,
        //					3, specified_pos,
        //&(param->unk_pos),
        // TYPE_HIRAGANA );
        //			if (specified_length && result_node)
        //				return result_node;
        //		}

        if (!specified_length && result_node) // only prediction
            find_reached_pos_of_pseudo_nodes(pos, result_node);
    }

    // 辞書にあったものと同じ品詞で同じ長さなら使わない
    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext) //末尾にシーク
            tmp_node = tmp_node->bnext;

        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            auto next_result = tmp_result_node->bnext;
            if (check_dict_match(tmp_result_node, dic_node)) {
                tmp_node->bnext = tmp_result_node;
                tmp_node = tmp_node->bnext;
            } else {
                delete tmp_result_node;
            }
            tmp_result_node = next_result;
        }
        tmp_node->bnext = nullptr;
        result_node = dic_node;
    }

    return result_node;
} //}}}

bool Sentence::lookup_and_analyze() { //{{{
    unsigned int previous_pos = 0;
    unsigned int char_num = 0;

    // 文字Lattice の構築
    CharLattice cl;
    cl.parse(sentence_c_str);
    Node::reset_id_count();

    // TODO:ラティスを作ったあと，空いている pos 間を埋める未定義語を生成
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        if ((*end_node_list)[pos] ==
            NULL) { // pos にたどり着くノードが１つもない
            if (param->unknown_word_detection && pos > 0 && reached_pos <= pos)
                make_unk_pseudo_node_list_from_previous_position(sentence_c_str,
                                                                 previous_pos);
        } else {
            // make figure/alphabet nodes and look up a dictionary
            // cerr << "char_num:" << char_num << ", byte:" << pos  << "<
            // length:" << length<< endl;
            Node *r_node = lookup_and_make_special_pseudo_nodes_lattice(
                cl, sentence_c_str, char_num, pos, 0, NULL);
            // Node *r_node =
            // lookup_and_make_special_pseudo_nodes(sentence_c_str, pos, 0,
            // NULL);

            // オノマトペ処理 or lookup_and_make_special_pseudo_nodes 内
            Node *r_onomatope_node =
                dic->recognize_onomatopoeia(sentence_c_str + pos);
            if (r_onomatope_node != NULL) {
                if (r_node) {
                    Node *tmp_node = r_node;
                    while (tmp_node->bnext)
                        tmp_node = tmp_node->bnext;
                    tmp_node->bnext = r_onomatope_node;
                } else {
                    // TODO:
                    // ここは修正しないと辞書にかすりもしないとオノマトペが使えない
                    // r_node = r_onomatope_node;
                }
            }

            set_begin_node_list(pos, r_node);
            find_reached_pos(pos, r_node);
            if (param->unknown_word_detection) { // make unknown word candidates
                if (reached_pos <= pos) {
                    // cerr << ";; Cannot connect at position:" << pos << " ("
                    // << in_sentence << ")" << endl;
                    r_node = make_unk_pseudo_node_list_from_some_positions(
                        sentence_c_str, pos, previous_pos);
                } else if (r_node && pos >= reached_pos_of_pseudo_nodes) {
                    for (unsigned int i = 2; i <= param->unk_max_length; i++) {
                        r_node = make_unk_pseudo_node_list_by_dic_check(
                            sentence_c_str, pos, r_node, i);
                    }
                    set_begin_node_list(pos, r_node);
                }
            }
            set_end_node_list(pos, r_node);
        }
        previous_pos = pos;
        char_num++;
    }
    if (param->debug)
        print_lattice();

    // Viterbi
    if (param->nbest) {
        for (unsigned int pos = 0; pos < length;
             pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
            viterbi_at_position_nbest(pos, (*begin_node_list)[pos]);
        }
        find_N_best_path();
    } else {
        for (unsigned int pos = 0; pos < length;
             pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
            viterbi_at_position(pos, (*begin_node_list)[pos]);
        }
        find_best_path();
    }
    return true;
} //}}}

void Sentence::print_lattice() { //{{{
    unsigned int char_num = 0;
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
        while (node) {
            for (unsigned int i = 0; i < char_num; i++)
                cerr << "  ";
            cerr << *(node->string_for_print);
            if (node->string_for_print != node->string)
                cerr << "(" << *(node->string) << ")";
            cerr << "_" << *(node->pos) << endl;
            node = node->bnext;
        }
        char_num++;
    }
} //}}}

void Sentence::print_juman_lattice() { //{{{

    mark_nbest();

    unsigned int char_num = 0;
    int id = 1;
    // 2 0 0 1 部屋 へや 部屋 名詞 6 普通名詞 1 * 0 * 0 “代表表記:部屋/へや
    // カテゴリ:場所-施設 …"
    // 15 2 2 2 に に に 助詞 9 格助詞 1 * 0 * 0 NIL
    // ID IDs_connecting_from index_begin index_end ...

    std::vector<std::vector<int>> num2id(length + 1); //多めに保持する
    // cout << "len:" << length << endl;
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];

        while (node) {
            size_t word_length = node->char_num;
            // cout << "pos:" << pos << " wordlen:" << word_length << endl;
            // ID の表示
            if (node->used_in_nbest) { // n-best解に使われているもののみ
                cout << id << " ";
                // 接続先
                num2id[char_num + word_length].push_back(id++);
                if (num2id[char_num].size() == 0) { // 無かったら 0 を出す
                    cout << "0";
                } else {
                    std::string sep = "";
                    for (auto to_id : num2id[char_num]) {
                        cout << sep << to_id;
                        sep = ";";
                    }
                }
                cout << " ";
                // 文字index の表示
                cout << char_num << " " << char_num + word_length - 1 << " ";

                // 表層 よみ 原形
                if (*node->reading == "*") { //読み不明であれば表層を使う
                    cout << *node->original_surface << " "
                         << *node->original_surface << " "
                         << *node->original_surface << " ";
                } else {
                    cout << *node->original_surface << " " << *node->reading
                         << " " << *node->base << " ";
                }
                if (*node->spos == UNK_POS) {
                    // 品詞 品詞id
                    cout << "未定義語"
                         << " " << Dic::pos_map.at("未定義語") << " ";
                    // 細分類 細分類id
                    cout << "その他 " << Dic::spos_map.at("その他") << " ";
                } else {
                    // 品詞 品詞id
                    cout << *node->pos << " " << Dic::pos_map.at(*node->pos)
                         << " ";
                    // 細分類 細分類id
                    cout << *node->spos << " " << Dic::spos_map.at(*node->spos)
                         << " ";
                }
                // 活用型 活用型id
                cout << *node->form_type << " "
                     << Dic::katuyou_type_map.at(*node->form_type) << " ";
                // 活用系 活用系id
                auto type_and_form = (*node->form_type + ":" + *node->form);
                if (Dic::katuyou_form_map.count(type_and_form) == 0) //無い場合
                    cout << *node->form << " " << 0 << " ";
                else
                    cout << *node->form << " "
                         << Dic::katuyou_form_map.at(type_and_form) << " ";

                // 意味情報を再構築して表示
                if (*node->representation != "*" ||
                    *node->semantic_feature != "NIL" ||
                    *node->spos == UNK_POS) {
                    std::string delim = "";
                    cout << '"';
                    if (*node->representation != "*") {
                        cout << "代表表記:"
                             << *node->representation; //*ならスキップ
                        delim = " ";
                    }
                    if (*node->semantic_feature != "NIL") {
                        cout << delim << *node->semantic_feature; // NILならNIL
                        delim = " ";
                    }
                    if (*node->spos == UNK_POS) {
                        cout << delim << "品詞推定:" << *node->pos << ":"
                             << *node->spos;
                        delim = " ";
                    }
                    U8string ustr(*node->original_surface);
                    // この辺りの処理はあとでくくり出してNode 生成時に行う
                    bool kieisuuka = false;
                    if (ustr.is_katakana()) {
                        cout << delim << "<カタカナ>";
                        delim = " ";
                        kieisuuka = true;
                    }
                    if (ustr.is_kanji()) {
                        cout << delim << "<漢字>";
                        delim = " ";
                    }
                    if (ustr.is_eisuu()) {
                        cout << delim << "<英数字>";
                        delim = " ";
                        kieisuuka = true;
                    }
                    if (ustr.is_kigou()) {
                        cout << delim << "<記号>";
                        delim = " ";
                        kieisuuka = true;
                    }
                    if (kieisuuka) {
                        cout << delim << "<記英数カ>";
                        delim = " ";
                    };
                    cout << '"' << endl;
                } else {
                    cout << "NIL" << endl;
                }
            }
            node = node->bnext;
        }
        char_num++;
    }
    cout << "EOS" << endl;
} //}}}

void Sentence::print_unified_lattice() { //{{{
    mark_nbest();

    unsigned int char_num = 0;
    // - 2 0 0 1 部屋 部屋/へや へや 部屋 名詞 6 普通名詞 1 * 0 * 0
    // "カテゴリ:場所-施設..."
    // - 15 2 2 2 に * に に 助詞 9 格助詞 1 * 0 * 0 NIL
    // wordmark ID fromIDs char_index_begin char_index_end surface rep_form
    // reading pos posid spos sposid form_type typeid form formid imis

    std::vector<std::vector<int>> num2id(length + 1); //多めに保持する
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
        const std::string wordmark{"-"};
        const std::string delim{"\t"};

        while (node) {
            size_t word_length = node->char_num;
            // ID の表示
            if (node->used_in_nbest) { // n-best解に使われているもののみ
                U8string ustr(*node->original_surface);
                cout << wordmark << delim << node->id << delim;
                num2id[char_num + word_length].push_back(
                    node->id); // 現在，接続先はn-best
                               // と関係なく繋がるもの全てを使用
                if (num2id[char_num].size() == 0) { // 無かったら 0 を出す
                    cout << "0";
                } else {
                    std::string sep = "";
                    for (auto to_id : num2id[char_num]) {
                        cout << sep << to_id;
                        sep = ";";
                    }
                }
                cout << delim;
                // 文字index の表示
                cout << char_num << delim << char_num + word_length - 1
                     << delim;

                // 表層 代表表記 よみ 原形
                if (*node->reading == "*"|| *node->reading == "<UNK>") { //読み不明であれば表層を使う
                    cout << *node->original_surface << delim
                         << *node->original_surface << "/" << *node->original_surface << delim // 擬似代表表記を表示する
                         << *node->original_surface << delim
                         << *node->original_surface << delim;
                } else {
                    if(*node->representation == "*"){ //Wikipedia 数詞など
                        cout << *node->original_surface << delim
                            << *node->original_surface << "/" << *node->reading << delim 
                            << *node->reading << delim 
                            << *node->base << delim;
                    }else{
                        cout << *node->original_surface << delim
                            << *node->representation << delim << *node->reading
                            << delim << *node->base << delim;
                    }
                }
                // 品詞 品詞id 細分類 細分類id
                if (*node->spos == UNK_POS) {
                    cout << "未定義語" << delim << Dic::pos_map.at("未定義語")
                         << delim;
                    if (ustr.is_katakana()) {
                        cout << "カタカナ" << delim
                             << Dic::spos_map.at("カタカナ") << delim;
                    } else if (ustr.is_alphabet()) {
                        cout << "アルファベット" << delim
                             << Dic::spos_map.at("アルファベット") << delim;
                    } else {
                        cout << "その他" << delim << Dic::spos_map.at("その他")
                             << delim;
                    }
                } else {
                    cout << *node->pos << delim << Dic::pos_map.at(*node->pos)
                         << delim;
                    cout << *node->spos << delim
                         << Dic::spos_map.at(*node->spos) << delim;
                }
                // 活用型 活用型id
                if (Dic::katuyou_type_map.count(*node->form_type) == 0) {
                    cout << "*" << delim << "0" << delim;
                } else {
                    cout << *node->form_type << delim
                         << Dic::katuyou_type_map.at(*node->form_type) << delim;
                }

                // 活用系 活用系id
                auto type_and_form = (*node->form_type + ":" + *node->form);
                if (Dic::katuyou_form_map.count(type_and_form) == 0) { //無い場合
                    if( *node->form == "<UNK>")
                        cout << "*" << delim << "0" << delim;
                    else
                        cout << *node->form << delim << "0" << delim;
                } else {
                    if( *node->form == "<UNK>"){
                        cout << "*" << delim << "0" << delim;
                    }else{
                        cout << *node->form << delim
                             << Dic::katuyou_form_map.at(type_and_form) << delim;
                    }
                }

                // 意味情報を再構築して表示
                if (*node->semantic_feature != "NIL" ||
                    *node->spos == UNK_POS || ustr.is_katakana() ||
                    ustr.is_kanji() || ustr.is_eisuu() || ustr.is_kigou()) {
                    const std::string sep = "|";
                    bool use_sep = false;

                    if (*node->semantic_feature != "NIL") {
                        // 一度空白で分割して表示する必要がある.
                        size_t current = 0, found;
                        while ((found = (*node->semantic_feature).find_first_of(
                                    ' ', current)) != std::string::npos) {
                            cout << (use_sep ? sep : "")
                                 << std::string(
                                        (*node->semantic_feature), current,
                                        found - current); // NILでなければ
                            current = found + 1;
                            use_sep = true;
                        }
                        if (current == 0) {
                            cout << (use_sep ? sep : "")
                                 << *node->semantic_feature;
                            use_sep = true;
                        }
                    }
                    if (*node->spos == UNK_POS) {
                        cout << (use_sep ? sep : "")
                             << "品詞推定:" << *node->pos << ":" << *node->spos;
                        use_sep = true;
                    }
                    // TODO:これ以降の処理はあとでくくり出してNode 生成時に行う
                    bool kieisuuka = false;
                    if (ustr.is_katakana()) {
                        cout << (use_sep ? sep : "") << "カタカナ";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (ustr.is_kanji()) {
                        cout << (use_sep ? sep : "") << "漢字";
                        use_sep = true;
                    }
                    if (ustr.is_eisuu()) {
                        cout << (use_sep ? sep : "") << "英数字";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (ustr.is_kigou()) {
                        cout << (use_sep ? sep : "") << "記号";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (kieisuuka) {
                        cout << (use_sep ? sep : "") << "記英数カ";
                    }
                    cout << endl;
                } else {
                    cout << "NIL" << endl;
                }

                if (param->debug) {
                    // デバッグ出力
                    cout << "!\twcost:" << node->wcost
                         << "\tcost:" << node->cost << endl;
                    cout << "!\t" << node->debug_info["unigram_feature"]
                         << endl;
                    std::stringstream ss_debug;
                    for (auto to_id : num2id[char_num]) {
                        ss_debug.str("");
                        ss_debug << to_id << " -> " << node->id;
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;
                    }
                    // BOS, EOS との接続の表示.. (TODO: 簡潔に書き換え)
                    ss_debug.str("");
                    ss_debug << -2 << " -> " << node->id;
                    if (node->debug_info.find(ss_debug.str()) !=
                        node->debug_info.end()) {
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                    }
                    ss_debug.str("");
                    ss_debug << node->id << " -> " << -1;
                    if (node->debug_info.find(ss_debug.str()) !=
                        node->debug_info.end()) {
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                    }
                }
            }
            node = node->bnext;
        }
        char_num++;
    }
    cout << "EOS" << endl;
} //}}}

Node *Sentence::get_bos_node() { //{{{
    Node *bos_node = new Node;
    bos_node->surface = BOS; // const_cast<const char *>(BOS);
    bos_node->string = new std::string(bos_node->surface);
    bos_node->string_for_print = new std::string(bos_node->surface);
    bos_node->end_string = new std::string(bos_node->surface);
    // bos_node->isbest = 1;
    bos_node->stat = MORPH_BOS_NODE;
    bos_node->posid = MORPH_DUMMY_POS;
    bos_node->pos = new std::string(BOS_STRING); // TODO: リーク
    bos_node->spos = new std::string(BOS_STRING);
    bos_node->form = new std::string(BOS_STRING);
    bos_node->form_type = new std::string(BOS_STRING);
    bos_node->base = new std::string(BOS_STRING);
    bos_node->id = -2;

    FeatureSet *f = new FeatureSet(ftmpl);
    f->extract_unigram_feature(bos_node);
    bos_node->wcost = f->calc_inner_product_with_weight();
    bos_node->feature = f;

    return bos_node;
} //}}}

Node *Sentence::get_eos_node() { //{{{
    Node *eos_node = new Node;
    eos_node->surface = EOS; // const_cast<const char *>(EOS);
    eos_node->string = new std::string(eos_node->surface);
    eos_node->string_for_print = new std::string(eos_node->surface);
    eos_node->end_string = new std::string(eos_node->surface);
    // eos_node->isbest = 1;
    eos_node->stat = MORPH_EOS_NODE;
    eos_node->posid = MORPH_DUMMY_POS;
    eos_node->pos = new std::string(EOS_STRING); // TODO: リーク
    eos_node->spos = new std::string(EOS_STRING);
    eos_node->form = new std::string(EOS_STRING);
    eos_node->form_type = new std::string(EOS_STRING);
    eos_node->base = new std::string(EOS_STRING);

    eos_node->id = -1;

    FeatureSet *f = new FeatureSet(ftmpl);
    f->extract_unigram_feature(eos_node);
    eos_node->wcost = f->calc_inner_product_with_weight();
    eos_node->feature = f;

    return eos_node;
} //}}}

// make EOS node and get the best path
Node *Sentence::find_best_path() {                 //{{{
    (*begin_node_list)[length] = (get_eos_node()); // End Of Sentence
    viterbi_at_position(length, (*begin_node_list)[length]);
    return (*begin_node_list)[length];
} //}}}

Node *Sentence::find_N_best_path() {             //{{{
    (*begin_node_list)[length] = get_eos_node(); // End Of Sentence
    viterbi_at_position_nbest(length, (*begin_node_list)[length]);
    return (*begin_node_list)[length];
} //}}}

void Sentence::set_begin_node_list(unsigned int pos, Node *new_node) { //{{{
    (*begin_node_list)[pos] = new_node;
} //}}}

// TODO:いずれ廃止 or nbest=1 の場合と統合
bool Sentence::viterbi_at_position(unsigned int pos, Node *r_node) { //{{{
    while (r_node) {
        double best_score = -DBL_MAX;
        Node *best_score_l_node = NULL;
        FeatureSet *best_score_bigram_f = NULL;
        Node *l_node = (*end_node_list)[pos];
        while (l_node) {
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_bigram_feature(l_node, r_node);
            double bigram_score = f->calc_inner_product_with_weight();
            double score = l_node->cost + bigram_score + r_node->wcost;

            if (score > best_score) {
                best_score_l_node = l_node;
                if (best_score_bigram_f)
                    delete best_score_bigram_f;
                best_score_bigram_f = f;
                best_score = score;
            } else {
                delete f;
            }
            l_node = l_node->enext;
        }

        if (best_score_l_node) {
            r_node->prev = best_score_l_node;
            r_node->next = NULL;
            r_node->cost = best_score;
            if (MODE_TRAIN) { // feature collection
                r_node->feature->append_feature(best_score_l_node->feature);
                r_node->feature->append_feature(best_score_bigram_f);
            }
            delete best_score_bigram_f;
        } else {
            return false;
        }

        r_node = r_node->bnext;
    }

    return true;
} //}}}

bool Sentence::viterbi_at_position_nbest(unsigned int pos, Node *r_node) { //{{{
    std::stringstream ss_key, ss_value;
    while (r_node) {
        std::priority_queue<NbestSearchToken> nodeHeap;
        Node *l_node = (*end_node_list)[pos];

        while (l_node) {
            // 濁音化の条件チェック
            if ((r_node->stat ==
                 MORPH_DEVOICE_NODE) && //今の形態素が濁音化している
                (!check_devoice_condition(
                     *l_node))) { //前の形態素が濁音化の条件を満たさない
                // std::cerr << *l_node->string_for_print << "=/=" <<
                // *r_node->string_for_print << std::endl;

                NbestSearchToken newSearchToken(-DBL_MAX, 0, l_node);
                nodeHeap.push(newSearchToken);
                l_node = l_node->enext;
                continue;
            }

            FeatureSet f(ftmpl);
            f.extract_bigram_feature(l_node, r_node);
            double bigram_score = f.calc_inner_product_with_weight();
            if (param->debug) {
                ss_key.str(""), ss_value.str("");
                ss_key << l_node->id << " -> " << r_node->id;
                ss_value << bigram_score << " + " << l_node->cost << " = "
                         << l_node->cost + bigram_score;
                r_node->debug_info[ss_key.str().c_str()] =
                    std::string(ss_value.str().c_str());
                l_node->debug_info[ss_key.str().c_str()] =
                    std::string(ss_value.str().c_str());
                r_node->debug_info[ss_key.str() + ":bigram_feature"] = f.str();
            }

            int traceSize = l_node->traceList.size();

            if (traceSize == 0) {
                double score = l_node->cost + bigram_score + r_node->wcost;
                NbestSearchToken newSearchToken(score, 0, l_node);
                nodeHeap.push(newSearchToken);

            } else {
                double last_score = DBL_MAX;
                for (int i = 0; i < traceSize; ++i) {
                    double score = l_node->traceList.at(i).score +
                                   bigram_score + r_node->wcost;
                    if (i > param->N_redundant && last_score > score)
                        break;
                    nodeHeap.emplace(score, i, l_node);
                    last_score = score;
                }
            }
            l_node = l_node->enext;
        }

        int heapSize = nodeHeap.size();

        double last_score = DBL_MAX;
        for (int i = 0; i < heapSize; ++i) {
            double score = nodeHeap.top().score;
            if (i > param->N_redundant && last_score > score)
                break;
            r_node->traceList.push_back(nodeHeap.top());
            nodeHeap.pop();
            last_score = score;
        }

        if (r_node->traceList.size() > 0) {
            r_node->next = NULL;
            r_node->prev = r_node->traceList.front().prevNode;
            r_node->cost = r_node->traceList.front().score;
        } else {
            return false;
        }

        r_node = r_node->bnext;
    }

    return true;
} //}}}

void Sentence::print_N_best_path() { //{{{
                                     // 曖昧性のある形態素の出力
    std::string output_string_buffer;

    unsigned int N_required = param->N;
    unsigned int N_couter = 0;

    unsigned int traceSize = (*begin_node_list)[length]->traceList.size();
    if (traceSize > param->N_redundant) {
        traceSize = param->N_redundant;
    }

    for (int i = 0; i < traceSize; ++i) {
        Node *node = (*begin_node_list)[length];
        std::vector<Node *> result_morphs;

        bool find_bos_node = false;
        int traceRank = i;

        Node *temp_node = NULL;
        //		std::string subword_buffer;
        long output_score = (*begin_node_list)[length]->traceList.at(i).score;
        //		cerr << node->traceList.at(i).rank << "\t" <<
        // node->traceList.at(i).score << endl;

        while (node) {
            result_morphs.push_back(node);

            if (node->traceList.size() == 0) {
                break;
            }
            node = node->traceList.at(traceRank).prevNode;
            if (node->stat == MORPH_BOS_NODE) {
                find_bos_node = true;
                break;
            } else {
                traceRank = result_morphs.back()->traceList.at(traceRank).rank;
            }
        }

        if (!find_bos_node)
            cerr << ";; cannot analyze:" << sentence << endl;

        size_t printed_num = 0;
        for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
             it != result_morphs.rend(); it++) {

            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;
                if (printed_num++)
                    output_string_buffer.append(" ");
                output_string_buffer.append(*(*it)->string_for_print);
                output_string_buffer.append("_");
                output_string_buffer.append(*(*it)->pos);
            }
        }

        std::map<std::string, int>::iterator find_output =
            nbest_duplicate_filter.find(output_string_buffer);
        if (find_output != nbest_duplicate_filter.end()) {
            // duplicate output
            //			cerr << "duplicate: " << output_string_buffer;
            //			cerr << " at " <<
            // nbest_duplicate_filter[output_string_buffer] << " vs. " << i <<
            // endl;
        } else {
            nbest_duplicate_filter.insert(
                std::make_pair(output_string_buffer, i));
            cout << "#" << output_score << endl;
            cout << output_string_buffer;
            cout << endl;
            ++N_couter;
        }

        output_string_buffer.clear();
        if (N_couter >= N_required)
            break;
    }
    cout << endl;
} //}}}

void Sentence::mark_nbest() { //{{{
    unsigned int traceSize_original =
        (*begin_node_list)[length]->traceList.size();
    unsigned int traceSize = (*begin_node_list)[length]->traceList.size();
    if (traceSize > param->N_redundant) {
        traceSize = param->N_redundant;
    }

    // 近いスコアの場合をまとめるために，整数に丸める
    double last_score = DBL_MAX;
    // double output_score = 0;
    long sample_num = 0;
    int i = 0;

    while (i < traceSize_original) {
        Node *node = (*begin_node_list)[length];
        std::vector<Node *> result_morphs;
        bool find_bos_node = false;
        int traceRank = i;

        Node *temp_node = NULL;
        double output_score = (*begin_node_list)[length]->traceList.at(i).score;
        if (last_score > output_score)
            sample_num++; // 同スコアの場合は数に数えず，N-bestに出力
        if (sample_num > param->N)
            break;
        // std::cerr << sample_num << ":"<< i << ":" << last_score << ":" <<
        // output_score << ":N " << traceSize << ":torig "<< traceSize_original
        // << std::endl;

        while (node) {
            result_morphs.push_back(node);

            if (node->traceList.size() == 0) {
                break;
            }
            node = node->traceList.at(traceRank).prevNode;
            if (node->stat == MORPH_BOS_NODE) {
                find_bos_node = true;
                break;
            } else {
                traceRank = result_morphs.back()->traceList.at(traceRank).rank;
            }
        }

        if (!find_bos_node) {
            cerr << ";; cannot analyze:" << sentence << endl;
        }

        size_t printed_num = 0;
        unsigned long byte_pos = 0;
        // 後ろから追加しているので、元の順にするために逆向き
        for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
             it != result_morphs.rend(); it++) {

            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest =
                    true; // Nodeにnbestに入っているかをマークするだけ

                if (output_ambiguous_word) {
                    auto tmp = (*begin_node_list)[byte_pos];
                    while (
                        tmp) { //同じ長さ(同じ表層)で同じ表層のものをすべて出力する
                        // cerr << "c:" << *tmp->string_for_print << ":" <<
                        // *tmp->pos << ":" << *tmp->representation << "," <<
                        // tmp->length << "," << (*it)->length << ", pos:" <<
                        // tmp->posid << "," << (*it)->posid << endl;
                        if (tmp->length == (*it)->length &&
                            tmp->posid == (*it)->posid) {
                            tmp->used_in_nbest = true;
                        }
                        tmp = tmp->bnext;
                    }
                }
                byte_pos += (*it)->length;
            }
        }
        i++;
        last_score = output_score;
    }
} //}}}

// update end_node_list
void Sentence::set_end_node_list(unsigned int pos, Node *r_node) { //{{{
    while (r_node) {
        if (r_node->stat != MORPH_EOS_NODE) {
            unsigned int end_pos = pos + r_node->length;
            r_node->enext = (*end_node_list)[end_pos];
            (*end_node_list)[end_pos] = r_node;
        }
        r_node = r_node->bnext;
    }
} //}}}

unsigned int Sentence::find_reached_pos(unsigned int pos, Node *node) { //{{{
    while (node) {
        unsigned int end_pos = pos + node->length;
        if (end_pos > reached_pos)
            reached_pos = end_pos;
        node = node->bnext;
    }
    return reached_pos;
} //}}}

// reached_pos_of_pseudo_nodes の計算、どこまでの範囲を未定義語として入れたか
unsigned int Sentence::find_reached_pos_of_pseudo_nodes(unsigned int pos,
                                                        Node *node) { //{{{
    while (node) {
        if (node->stat == MORPH_UNK_NODE) {
            unsigned int end_pos = pos + node->length;
            if (end_pos > reached_pos_of_pseudo_nodes)
                reached_pos_of_pseudo_nodes = end_pos;
        }
        node = node->bnext;
    }
    return reached_pos_of_pseudo_nodes;
} //}}}

void Sentence::print_best_path() { //{{{
    Node *node = (*begin_node_list)[length];
    std::vector<Node *> result_morphs;

    bool find_bos_node = false;
    while (node) {
        if (node->stat == MORPH_BOS_NODE)
            find_bos_node = true;
        result_morphs.push_back(node);
        node = node->prev;
    }

    if (!find_bos_node)
        cerr << ";; cannot analyze:" << sentence << endl;

    size_t printed_num = 0;
    for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
         it != result_morphs.rend(); it++) {
        if ((*it)->stat != MORPH_BOS_NODE && (*it)->stat != MORPH_EOS_NODE) {
            if (printed_num++)
                cout << " ";
            (*it)->print();
        }
    }
    cout << endl;
} //}}}

void Sentence::minus_feature_from_weight(
    std::unordered_map<std::string, double> &in_feature_weight,
    size_t factor) {                         //{{{
    Node *node = (*begin_node_list)[length]; // EOS
    node->feature->minus_feature_from_weight(in_feature_weight, factor);
} //}}}

void Sentence::minus_feature_from_weight(
    std::unordered_map<std::string, double> &in_feature_weight) { //{{{
    minus_feature_from_weight(in_feature_weight, 1);
} //}}}

bool Sentence::lookup_gold_data(std::string &word_pos_pair) { //{{{
    if (reached_pos < length) {
        cerr << ";; ERROR! Cannot connect at position for gold: "
             << word_pos_pair << endl;
    }

    std::vector<std::string> line(7);
    split_string(word_pos_pair, "_", line);
    std::vector<std::string> spec{
        line[1], line[2], line[3],
        line[4], line[5], line[6]}; // reading,base,pos, spos, formtype, form

    // そのまま辞書を引く
    Node *r_node = lookup_and_make_special_pseudo_nodes(
        line[0].c_str(), strlen(line[0].c_str()), spec);

    //
    if (!r_node &&
        line[6] ==
            "命令形エ形") { // JUMANの活用になく，３文コーパスでのみ出てくる活用の命令形エ形を連用形だと解釈する.
        std::vector<std::string> mod_spec{line[1], "",      line[3],
                                          line[4], line[5], "基本連用形"};
        r_node = lookup_and_make_special_pseudo_nodes(
            line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore 命令形エ形 verb:" <<  line[0] << ":" << line[1] <<
        // ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" << line[5]
        // << ":" << line[6] << endl;
    }
    if (!r_node && line[3] == "名詞") { // 動詞の名詞化を復元
        // コーパスでは名詞化した動詞は名詞として扱われる. (kkn
        // では動詞の連用形扱い)
        // 品詞変更タグがついていない場合の対策
        // 動詞の活用型不明のまま，基本連用形をチェック
        std::vector<std::string> mod_spec{line[1], line[2], "動詞",
                                          "*",     "",      "基本連用形"};
        r_node = lookup_and_make_special_pseudo_nodes(
            line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore nominalized verb:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }
    // 人名, 地名が辞書にない場合 // 未定義語として処理

    if (!r_node) { // 濁音化, 音便化している(?相:しょう,など)しているケース
        // は読みを無視して検索
        std::vector<std::string> mod_spec{"",      line[2], line[3],
                                          line[4], line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes(
            line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore voiced reading:" <<  line[0] << ":" << line[1] <<
        // ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" << line[5]
        // << ":" << line[6] << endl;
    }

    // 名詞が固有名詞, サ変名詞化している場合, 細分類を無視して辞書引き
    if (!r_node && line[3] == "名詞") {
        std::vector<std::string> mod_spec{line[1], line[2], line[3],
                                          "",      line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes(
            line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; modify proper noun to noun:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }

    // 記号に読み方が書かれている場合は無視する
    if (!r_node && line[3] == "特殊") {
        std::vector<std::string> mod_spec{"",      line[2], line[3],
                                          line[4], line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes(
            line[0].c_str(), strlen(line[0].c_str()), mod_spec);
    }

    if (!r_node) { // 未定義語として処理
        // 細分類以下まで推定するなら，以下も書き換える
        r_node = dic->make_unk_pseudo_node_gold(
            line[0].c_str(), strlen(line[0].c_str()), line[3]);
        cerr << ";; lookup failed in gold data:" << line[0] << ":" << line[1]
             << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":"
             << line[5] << ":" << line[6] << endl;
    }
    (*begin_node_list)[length] = r_node;
    find_reached_pos(length, r_node);
    viterbi_at_position(length, r_node);
    set_end_node_list(length, r_node);

    add_one_word(line[0]);
    return true;
} //}}}

double Sentence::eval(Sentence &gold) { //{{{
    if (length != gold.length) {
        cerr << ";; cannot calc loss " << sentence << endl;
        return 1.0;
    }

    Node *node = (*begin_node_list)[length];
    std::vector<Node *> result_morphs;

    bool find_bos_node = false;
    while (node) { // 1-best
        if (node->stat == MORPH_BOS_NODE)
            find_bos_node = true;
        result_morphs.push_back(node);
        node = node->prev;
    }

    if (!find_bos_node)
        cerr << ";; cannot analyze:" << sentence << endl;

    auto itr = result_morphs.rbegin();
    auto itr_gold = gold.gold_morphs.rbegin();

    size_t byte = 0;
    size_t byte_gold = 0;
    double score = 0.0;
    size_t morph_count = 0;
    // while (itr_gold != gold.gold_morphs.rend() && itr !=
    // result_morphs.rend()){
    while (itr_gold != gold.gold_morphs.rend()) {
        if (byte_gold > byte) {
            byte += (*itr)->length;
            itr++;
        } else if (byte > byte_gold) {
            byte_gold += (itr_gold)->length;
            itr_gold++;
            morph_count++;
        } else { // byte == byte_gold
            // 同じ場所に同じ長さがあればスコア
            if ((*itr)->length == (itr_gold)->length) {
                //同じ品詞 なら追加で0.5点
                if ((*itr)->posid == (itr_gold)->posid &&
                    (*itr)->sposid == (itr_gold)->sposid)
                    score += 1.0;
                else
                    score += 0.5;
            }
            // cerr << *(*itr)->base << " "<< *(itr_gold)->base << endl;
            byte += (*itr)->length;
            byte_gold += (itr_gold)->length;
            itr++;
            itr_gold++;
            morph_count++;
        }
    }
    // TODO:BOS EOS を除く
    // return 1.0;
    return 1.0 - (score / morph_count);
}; //}}}
}

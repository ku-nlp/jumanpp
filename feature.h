#pragma once
#ifndef FEATURE_H
#define FEATURE_H

#include "common.h"
#include "node.h"
#include "parameter.h"
#include <sstream>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <functional>

namespace Morph {


// どのノードについての素性関数であるか.
enum NodePosition {LeftN,RightN,CenterN,UnigramN};

// ------ UNIGRAM ------
#define FEATURE_MACRO_STRING_WORD "%w"
#define FEATURE_MACRO_WORD 1
#define FEATURE_MACRO_STRING_POS "%p"
#define FEATURE_MACRO_POS 2
#define FEATURE_MACRO_STRING_LENGTH "%l"
#define FEATURE_MACRO_LENGTH 3
#define FEATURE_MACRO_STRING_BEGINNING_CHAR "%bc"
#define FEATURE_MACRO_BEGINNING_CHAR 4
#define FEATURE_MACRO_STRING_ENDING_CHAR "%ec"
#define FEATURE_MACRO_ENDING_CHAR 5
#define FEATURE_MACRO_STRING_BEGINNING_CHAR_TYPE "%bt"
#define FEATURE_MACRO_BEGINNING_CHAR_TYPE 6
#define FEATURE_MACRO_STRING_ENDING_CHAR_TYPE "%et"
#define FEATURE_MACRO_ENDING_CHAR_TYPE 7
#define FEATURE_MACRO_STRING_FEATURE1 "%f1"
#define FEATURE_MACRO_FEATURE1 8

#define FEATURE_MACRO_STRING_SPOS "%sp"
#define FEATURE_MACRO_SPOS 11
#define FEATURE_MACRO_STRING_FORM "%sf"
#define FEATURE_MACRO_FORM 12
#define FEATURE_MACRO_STRING_FORM_TYPE "%sft"
#define FEATURE_MACRO_FORM_TYPE 13
#define FEATURE_MACRO_STRING_FUNCTIONAL_WORD "%f"
#define FEATURE_MACRO_FUNCTIONAL_WORD 14
#define FEATURE_MACRO_STRING_BASE_WORD "%ba"
#define FEATURE_MACRO_BASE_WORD 15

#define FEATURE_MACRO_STRING_DEVOICE  "%devoice"
#define FEATURE_MACRO_DEVOICE  17
#define FEATURE_MACRO_STRING_LONGER  "%longer"
#define FEATURE_MACRO_LONGER  19
#define FEATURE_MACRO_STRING_NUMSTR  "%numstr"
#define FEATURE_MACRO_NUMSTR  20
#define FEATURE_MACRO_STRING_LEXICAL  "%lexical"
#define FEATURE_MACRO_LEXICAL  21

//------ LEFT ------
#define FEATURE_MACRO_STRING_LEFT_WORD "%Lw"
#define FEATURE_MACRO_LEFT_WORD 101
#define FEATURE_MACRO_STRING_LEFT_POS "%Lp"
#define FEATURE_MACRO_LEFT_POS 102
#define FEATURE_MACRO_STRING_LEFT_LENGTH "%Ll"
#define FEATURE_MACRO_LEFT_LENGTH 103
#define FEATURE_MACRO_STRING_LEFT_BEGINNING_CHAR "%Lbc"
#define FEATURE_MACRO_LEFT_BEGINNING_CHAR 104
#define FEATURE_MACRO_STRING_LEFT_ENDING_CHAR "%Lec"
#define FEATURE_MACRO_LEFT_ENDING_CHAR 105
#define FEATURE_MACRO_STRING_LEFT_BEGINNING_CHAR_TYPE "%Lbt"
#define FEATURE_MACRO_LEFT_BEGINNING_CHAR_TYPE 106
#define FEATURE_MACRO_STRING_LEFT_ENDING_CHAR_TYPE "%Let"
#define FEATURE_MACRO_LEFT_ENDING_CHAR_TYPE 107

#define FEATURE_MACRO_STRING_LEFT_SPOS "%Lsp"
#define FEATURE_MACRO_LEFT_SPOS 111
#define FEATURE_MACRO_STRING_LEFT_FORM "%Lsf"
#define FEATURE_MACRO_LEFT_FORM 112
#define FEATURE_MACRO_STRING_LEFT_FORM_TYPE "%Lsft"
#define FEATURE_MACRO_LEFT_FORM_TYPE 113

#define FEATURE_MACRO_STRING_LEFT_FUNCTIONAL_WORD "%Lf"
#define FEATURE_MACRO_LEFT_FUNCTIONAL_WORD  114
#define FEATURE_MACRO_STRING_LEFT_BASE_WORD "%Lba"
#define FEATURE_MACRO_LEFT_BASE_WORD  115

#define FEATURE_MACRO_STRING_LEFT_PREFIX "%Lprefix"
#define FEATURE_MACRO_LEFT_PREFIX  116
#define FEATURE_MACRO_STRING_LEFT_SUFFIX "%Lsuffix"
#define FEATURE_MACRO_LEFT_SUFFIX  117
#define FEATURE_MACRO_STRING_LEFT_DUMMY "%Ldummy"
#define FEATURE_MACRO_LEFT_DUMMY 118
#define FEATURE_MACRO_STRING_LEFT_LONGER "%Llonger"
#define FEATURE_MACRO_LEFT_LONGER 119
#define FEATURE_MACRO_STRING_LEFT_NUMSTR "%Lnumstr"
#define FEATURE_MACRO_LEFT_NUMSTR 120
#define FEATURE_MACRO_STRING_LEFT_LEXICAL  "%Llexical"
#define FEATURE_MACRO_LEFT_LEXICAL  121

//------ RIGHT ------
#define FEATURE_MACRO_STRING_RIGHT_WORD "%Rw"
#define FEATURE_MACRO_RIGHT_WORD 201
#define FEATURE_MACRO_STRING_RIGHT_POS "%Rp"
#define FEATURE_MACRO_RIGHT_POS 202
#define FEATURE_MACRO_STRING_RIGHT_LENGTH "%Rl"
#define FEATURE_MACRO_RIGHT_LENGTH 203
#define FEATURE_MACRO_STRING_RIGHT_BEGINNING_CHAR "%Rbc"
#define FEATURE_MACRO_RIGHT_BEGINNING_CHAR 204
#define FEATURE_MACRO_STRING_RIGHT_ENDING_CHAR "%Rec"
#define FEATURE_MACRO_RIGHT_ENDING_CHAR 205
#define FEATURE_MACRO_STRING_RIGHT_BEGINNING_CHAR_TYPE "%Rbt"
#define FEATURE_MACRO_RIGHT_BEGINNING_CHAR_TYPE 206
#define FEATURE_MACRO_STRING_RIGHT_ENDING_CHAR_TYPE "%Ret"
#define FEATURE_MACRO_RIGHT_ENDING_CHAR_TYPE 207

#define FEATURE_MACRO_STRING_RIGHT_SPOS "%Rsp"
#define FEATURE_MACRO_RIGHT_SPOS 211
#define FEATURE_MACRO_STRING_RIGHT_FORM "%Rsf"
#define FEATURE_MACRO_RIGHT_FORM 212
#define FEATURE_MACRO_STRING_RIGHT_FORM_TYPE "%Rsft"
#define FEATURE_MACRO_RIGHT_FORM_TYPE 213

#define FEATURE_MACRO_STRING_RIGHT_FUNCTIONAL_WORD "%Rf"
#define FEATURE_MACRO_RIGHT_FUNCTIONAL_WORD  214
#define FEATURE_MACRO_STRING_RIGHT_BASE_WORD "%Rba"
#define FEATURE_MACRO_RIGHT_BASE_WORD  215

#define FEATURE_MACRO_STRING_RIGHT_PREFIX "%Rprefix"
#define FEATURE_MACRO_RIGHT_PREFIX 216
#define FEATURE_MACRO_STRING_RIGHT_SUFFIX "%Rsuffix"
#define FEATURE_MACRO_RIGHT_SUFFIX 217

#define FEATURE_MACRO_STRING_RIGHT_DUMMY "%Rdummy"
#define FEATURE_MACRO_RIGHT_DUMMY  218
#define FEATURE_MACRO_STRING_RIGHT_LONGER "%Rlonger"
#define FEATURE_MACRO_RIGHT_LONGER 219
#define FEATURE_MACRO_STRING_RIGHT_NUMSTR "%Rnumstr"
#define FEATURE_MACRO_RIGHT_NUMSTR 220

#define FEATURE_MACRO_STRING_RIGHT_LEXICAL  "%Rlexical"
#define FEATURE_MACRO_RIGHT_LEXICAL  221

//----- middle  ----- (left middle right) の並び
//とりあえず, middle は unigram のテンプレートを流用する. 

#define TOPIC_NUM 50
#define NUM_OF_FUKUGOUJI 39

// unordered_map で tuple を扱うための hash 関数
struct tuple_hash : public std::unary_function<std::tuple<std::string, std::string, std::string, std::string>, std::size_t>
{
    std::size_t operator()(const std::tuple<std::string, std::string, std::string, std::string>& k) const
    { 
        std::hash<std::string> hash_str;
        // このハッシュだと衝突が多そう?
        return hash_str(std::get<0>(k)) ^ hash_str(std::get<1>(k)) ^ hash_str(std::get<2>(k)) ^ hash_str(std::get<3>(k));
    }
};
 
struct tuple_equal : public std::binary_function<std::tuple<std::string, std::string, std::string, std::string>, std::tuple<std::string, std::string, std::string, std::string>, bool>
{
    bool operator()(const std::tuple<std::string, std::string, std::string, std::string>& v0, const std::tuple<std::string, std::string, std::string, std::string>& v1) const
    {
        return (std::get<0>(v0) == std::get<0>(v1) &&
                std::get<1>(v0) == std::get<1>(v1) &&
                std::get<2>(v0) == std::get<2>(v1) &&
                std::get<3>(v0) == std::get<3>(v1) );
    }
};

// 1-3gram のある素性に対応するクラス
class FeatureTemplate {//{{{

  public:
    // 素性関数オブジェクト
    typedef unsigned long FHash;
    typedef std::vector<FHash> FTemp;
    typedef std::function<void(Node*,Node*,Node*,FTemp&)> FeatureFunction;

  private:
    bool is_unigram;
    bool is_bigram;
    bool is_trigram;
    std::string name;
    unsigned long feature_name_hash;

//    // 要素ごとの素性化関数の配列
//    std::vector<FeatureFunction> feature_functions;
            
//    // 素性名と素性関数の対応map 保留
//    static std::unordered_map<std::string, std::function<void(Node*,FeatureTemplate::FTemp&,NodePosition)>> feature_map_function;

//    // 素性名と素性IDの対応map 
//    static std::unordered_map<std::string,unsigned long> fname2fid;
//    static std::unordered_map<unsigned long,std::string> fid2fname;
    
//    // bind化のコストが思ったよりも高いので保留
//    FeatureFunction interpret_macro_function(std::string &macro); 
        
    std::vector<unsigned int> features;
  public:
    FeatureTemplate(std::string &in_name, std::string &feature_string, int in_n_gram);
    bool get_is_unigram() {
        return is_unigram;
    }
    bool get_is_bigram() {
        return is_bigram;
    }
    bool get_is_trigram() {
        return is_trigram;
    }
    
//    // 保留
//    const std::vector<FeatureFunction>& get_feature_functions() { return feature_functions; }

    std::string &get_name() { return name; }
    unsigned long &get_name_hash() { return feature_name_hash; }
    
    unsigned int interpret_macro(std::string &macro);
    std::vector<unsigned int> *get_features() { return &features; }
}; //}}}

class FeatureTemplateSet {//{{{
    std::vector<FeatureTemplate *> templates;
  public:
    FeatureVector* set_weight;
    bool open(const std::string &template_filename);
    FeatureTemplate *interpret_template(std::string &template_string, int n_gram);
    std::vector<FeatureTemplate *> *get_templates() {
        return &templates;
    }
};//}}}

// 使っていないメンバの削除
// weight をよそに移譲
class FeatureSet { //{{{
  friend int ::main(int argc, char** argv);
    FeatureTemplateSet *ftmpl;
    FeatureVector* weight;
  private:
    static std::unordered_map<long int,std::string> feature_map; //sub_feature_map
    static bool debug_flag;
    static bool use_total_sim;

  public: 
    static std::vector<double>* topic;
    static bool open_freq_word_set(const std::string &list_filename); 
    static std::unordered_set<std::tuple<std::string, std::string, std::string, std::string>, tuple_hash, tuple_equal> freq_word_set;
        
    std::vector<std::string> fset;
    FeatureVector fvec; //暫定
        
    FeatureSet(FeatureTemplateSet *in_ftmpl);
    FeatureSet(const FeatureSet& f){
        ftmpl = f.ftmpl;
        weight = f.weight;
        fset = f.fset;
        fvec = f.fvec; //
    };
    ~FeatureSet();
    void clear(){fset.clear();};
    double calc_inner_product_with_weight();
    void extract_unigram_feature(Node *node);
    void extract_topic_feature(Node *node);
    void extract_bigram_feature(Node *l_node, Node *r_node);
    void extract_trigram_feature(Node *l_node, Node *m_node, Node *r_node);

    bool append_feature(FeatureSet *in);

    // 廃止予定
    //inline std::vector<std::string>& get_fset(){return fset;};

    // 廃止予定
    inline long int get_feature_id(const std::string& s){
        std::hash<std::string> hash_func;
        return hash_func(s);
    };

    // 表示用
    inline FeatureVector& get_fset(){return fvec;};
    bool print();
    std::string str();
    std::string binning(double x){//{{{
        // 完全に0, 0.01 以下，0.05以下, 0.1 以下, 0.2以下, 0.3以下, 0.4, 0.5, 0.6, 0.7,0.8,0.9,それ以上, の１３通り x 符号
        
        if(x == 0.0)
            return std::string("0");
        else if (x <= -1.0)
            return std::string("-1.0");
        else if (x <= -0.9)
            return std::string("-0.9");
        else if (x <= -0.8)
            return std::string("-0.8");
        else if (x <= -0.7)
            return std::string("-0.7");
        else if (x <= -0.6)
            return std::string("-0.6");
        else if (x <= -0.5)
            return std::string("-0.5");
        else if (x <= -0.4)
            return std::string("-0.4");
        else if (x <= -0.3)
            return std::string("-0.3");
        else if (x <= -0.2)
            return std::string("-0.2");
        else if (x <= -0.1)
            return std::string("-0.1");
        else if (x <= -0.05)
            return std::string("-0.05");
        else if (x <= -0.01)
            return std::string("-0.01");
        else if (x <= 0.01)
            return std::string("0.01");
        else if (x <= 0.05)
            return std::string("0.05");
        else if (x <= 0.1)
            return std::string("0.1");
        else if (x <= 0.2)
            return std::string("0.2");
        else if (x <= 0.3)
            return std::string("0.3");
        else if (x <= 0.4)
            return std::string("0.4");
        else if (x <= 0.5)
            return std::string("0.5");
        else if (x <= 0.6)
            return std::string("0.6");
        else if (x <= 0.7)
            return std::string("0.7");
        else if (x <= 0.7)
            return std::string("0.7");
        else if (x <= 0.8)
            return std::string("0.8");
        else if (x <= 0.9)
            return std::string("0.9");
        else 
            return std::string("1.0");
    }//}}}

  private:
};

}//}}}

#endif

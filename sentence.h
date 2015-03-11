#ifndef SENTENCE_H
#define SENTENCE_H

#include "common.h"

#include "feature.h"
#include "dic.h"
#include "parameter.h"
#include "charlattice.h"
#include "u8string.h"
#include <math.h>

namespace Morph {

class Sentence {
    Parameter *param;
    Dic *dic;
    FeatureTemplateSet *ftmpl;
    bool unknown_word_detection;
    std::vector<Node> gold_morphs; //正例の場合のみ利用
        
    size_t word_num;
    unsigned int length; // length of this sentence
    std::string sentence;
    const char *sentence_c_str;
    FeatureSet *feature;
    std::vector<Node *> *begin_node_list; // position -> list of nodes that begin at this pos
    std::vector<Node *> *end_node_list;   // position -> list of nodes that end at this pos
    unsigned int reached_pos;
    unsigned int reached_pos_of_pseudo_nodes;
    bool output_ambiguous_word;
  public:
    Sentence(std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, std::string &in_sentence, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    Sentence(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    void init(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    ~Sentence();
    void clear_nodes();
    void set_gold_nodes(){//{{{
        Node* node_gold = (*begin_node_list)[length];
        bool find_bos_node_gold = false;
        while (node_gold) {
            if (node_gold->stat == MORPH_BOS_NODE)
                find_bos_node_gold = true;
            Node new_node; 
            new_node.stat = node_gold->stat;
            new_node.posid = node_gold->posid;
            new_node.pos = node_gold->pos;
            new_node.sposid = node_gold->sposid;
            new_node.spos = node_gold->spos;
            new_node.imisid = node_gold->imisid;
            new_node.semantic_feature = node_gold->semantic_feature;

            //new_node.surface = node_gold->surface; //無意味
            //new_node.string_for_print = node_gold->string_for_print; //エラー
            new_node.base = node_gold->base;
            new_node.representation = node_gold->representation;
            new_node.length = node_gold->length;
            new_node.char_num = node_gold->char_num;
            gold_morphs.push_back(new_node);
            node_gold = node_gold->prev;
        }
        if(!find_bos_node_gold)
            cerr << ";; gold parse err"<< endl;
    }//}}}

    std::vector<std::string> get_gold_topic_features(TopicVector *tov);

    bool lookup_and_analyze();
    bool lookup();
    bool analyze();
    bool add_one_word(std::string &word);
    std::string &get_sentence() {
        return sentence;
    }
    FeatureSet *get_feature() {//{{{
        return feature;
    }//}}}
    FeatureSet *set_feature() {//{{{
        if (feature)
            delete feature;
        feature = (*begin_node_list)[length]->feature;
        (*begin_node_list)[length]->feature = NULL;
        return feature;
    }//}}}
    void feature_print();
    unsigned int get_length() {
        return length;
    }

    bool check_devoice_condition(Node& node){ //次の形態素が濁音化できるかどうかのチェック//{{{
        /* 濁音化するのは直前の形態素が名詞、または動詞の連用形、名詞性接尾辞の場合のみ */
        /* 接尾辞の場合を除き直前の形態素が平仮名1文字となるものは不可 */
        /* 連濁は文の先頭は不可 */
        
        bool check_verb = (node.posid == dic->posid2pos.get_id("動詞") && 
                          (node.formid == dic->formid2form.get_id("基本連用形")||
                           node.formid == dic->formid2form.get_id("タ接連用形"))); // 動詞で連用形. Juman の時点で，連用形は母音動詞の連用形と同じid かどうかだけをチェックしている
        bool check_noun = (node.posid == dic->posid2pos.get_id("名詞") && 
                          node.sposid != dic->sposid2spos.get_id("形式名詞")); // 形式名詞以外の名詞
        bool check_postp = (node.posid == dic->posid2pos.get_id("接尾辞") && 
                           ( node.sposid == dic->sposid2spos.get_id("名詞性述語接尾辞") || 
                             node.sposid == dic->sposid2spos.get_id("名詞性名詞接尾辞") || 
                             node.sposid == dic->sposid2spos.get_id("名詞性名詞助数辞") || 
                             node.sposid == dic->sposid2spos.get_id("名詞性特殊接尾辞") )); //接尾辞
        bool check_hiragana =  (!(node.posid != dic->posid2pos.get_id("接尾辞") && 
                               node.char_type == TYPE_HIRAGANA && 
                               node.char_num == 1)); //接尾辞以外のひらがな一文字でない
        bool check_bos = !(node.stat & MORPH_BOS_NODE); //文の先頭でない
        ///* カタカナ複合語の連濁は認めない */ //未実装

        return ( check_bos & check_hiragana & ( check_verb | check_noun | check_postp));
    };//}}}

    FeatureSet& get_best_feature() {//{{{
        Node *node = (*begin_node_list)[length]; // EOS
        return *node->feature;
    }//}}}

    Node *get_bos_node();
    Node *get_eos_node();
    Node *find_best_path();
    void set_begin_node_list(unsigned int pos, Node *new_node);
    void set_end_node_list(unsigned int pos, Node *r_node);
    bool viterbi_at_position(unsigned int pos, Node *r_node);
	bool viterbi_at_position_nbest(unsigned int pos, Node *r_node);
    unsigned int find_reached_pos(unsigned int pos, Node *node);
    unsigned int find_reached_pos_of_pseudo_nodes(unsigned int pos, Node *node);
    void print_best_path();
    TopicVector get_topic();
    void print_lattice();
	std::map<std::string, int> nbest_duplicate_filter;
	void print_N_best_path();
    void mark_nbest();
    Node* find_N_best_path(); // make EOS node and get N-best path
    double eval(Sentence& gold);
    void print_juman_lattice(); // 互換性の為
    void print_unified_lattice(); 
    void minus_feature_from_weight(std::unordered_map<std::string, double> &in_feature_weight);
    void minus_feature_from_weight(std::unordered_map<std::string, double> &in_feature_weight, size_t factor);
    bool lookup_gold_data(std::string &word_pos_pair);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int specified_length, const std::vector<std::string>& spec);
    Node *lookup_and_make_special_pseudo_nodes_lattice(CharLattice &cl, const char *start_str, unsigned int char_num, unsigned int pos, unsigned int specified_length, std::string *specified_pos); 
    bool check_dict_match(Node* tmp_node, Node* dic_node);
    
    Node *make_unk_pseudo_node_list_from_previous_position(const char *start_str, unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_from_some_positions(const char *start_str, unsigned int pos, unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_by_dic_check(const char *start_str, unsigned int pos, Node *r_node, unsigned int specified_char_num);
};



}

#endif

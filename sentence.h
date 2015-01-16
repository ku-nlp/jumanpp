#ifndef SENTENCE_H
#define SENTENCE_H

#include "common.h"

#include "feature.h"
#include "dic.h"
#include "parameter.h"
#include "charlattice.h"

namespace Morph {

class Sentence {
    Parameter *param;
    Dic *dic;
    FeatureTemplateSet *ftmpl;
    bool unknown_word_detection;

    size_t word_num;
    unsigned int length; // length of this sentence
    std::string sentence;
    const char *sentence_c_str;
    FeatureSet *feature;
    std::vector<Node *> *begin_node_list; // position -> list of nodes that begin at this pos
    std::vector<Node *> *end_node_list;   // position -> list of nodes that end at this pos
    unsigned int reached_pos;
    unsigned int reached_pos_of_pseudo_nodes;
  public:
    Sentence(std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, std::string &in_sentence, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    Sentence(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    void init(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    ~Sentence();
    void clear_nodes();
    bool lookup_and_analyze();
    bool add_one_word(std::string &word);
    std::string &get_sentence() {
        return sentence;
    }
    FeatureSet *get_feature() {
        return feature;
    }
    FeatureSet *set_feature() {
        if (feature)
            delete feature;
        feature = (*begin_node_list)[length]->feature;
        (*begin_node_list)[length]->feature = NULL;
        return feature;
    }
    void feature_print();
    unsigned int get_length() {
        return length;
    }
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
    void print_lattice();
	std::map<std::string, int> nbest_duplicate_filter;
	void print_N_best_path();
    void mark_nbest();
    // make EOS node and get N-best path
    Node* find_N_best_path() {
        (*begin_node_list)[length] = get_eos_node(); // End Of Sentence
        viterbi_at_position_nbest(length, (*begin_node_list)[length]);
        return (*begin_node_list)[length];
    }
    void print_juman_lattice();
    void minus_feature_from_weight(std::map<std::string, double> &in_feature_weight);
    void minus_feature_from_weight(std::map<std::string, double> &in_feature_weight, size_t factor);
    bool lookup_gold_data(std::string &word_pos_pair);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes_lattice(CharLattice &cl, const char *start_str, unsigned int char_num, unsigned int pos, unsigned int specified_length, std::string *specified_pos); 

    Node *make_unk_pseudo_node_list_from_previous_position(const char *start_str, unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_from_some_positions(const char *start_str, unsigned int pos, unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_by_dic_check(const char *start_str, unsigned int pos, Node *r_node, unsigned int specified_char_num);
};



}

#endif

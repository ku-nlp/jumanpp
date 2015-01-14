#ifndef TAGGER_H
#define TAGGER_H

#include "common.h"
#include "dic.h"
#include "sentence.h"
#include "feature.h"
#include "parameter.h"

namespace Morph {

class Tagger {
    Parameter *param;
    Dic dic;
    FeatureTemplateSet ftmpl;
    bool unknown_word_detection;
    bool shuffle_training_data;

    Sentence *sentence; // current input sentence

    size_t iteration_num;
    int sentences_for_train_num;
    std::vector<Sentence *> sentences_for_train;
         
    std::vector<Node *> begin_node_list; // position -> list of nodes that begin at this pos
    std::vector<Node *> end_node_list;   // position -> list of nodes that end at this pos
  public:
    Tagger(Parameter *in_param);
    ~Tagger();
    Sentence *new_sentence_analyze(std::string &in_sentence);
    void sentence_clear();
    bool viterbi_at_position(unsigned int pos, Node *r_node);
    void print_best_path();
    void print_N_best_path();
    void print_lattice();

    bool train(const std::string &gsd_file);
    bool read_gold_data(const std::string &gsd_file);
    bool read_gold_data(const char *gsd_file);
    bool lookup_gold_data(Sentence *sentence, std::string &word_pos_pair);
    bool add_one_sentence_for_train(Sentence *in_sentence);
    void clear_gold_data();
};

}

#endif

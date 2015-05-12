#ifndef TAGGER_H
#define TAGGER_H

#include "common.h"
#include "dic.h"
#include "sentence.h"
#include "feature.h"
#include "parameter.h"
#include "scw.h"

namespace Morph {

class Tagger {
// TODO: topic 関連はまとめる
    Parameter *param;
    Dic dic;
    FeatureTemplateSet ftmpl;
    bool unknown_word_detection;
    bool shuffle_training_data;
        
    FeatureVector weight; 
    FeatureVector weight_sum; 
    SCWClassifier scw;
    unsigned int num_of_sentences_processed = 0; //static?
    unsigned int total_iteration_num = 0;
    std::unordered_map<std::string, double> feature_weight_sum;
        
    Sentence *sentence; // current input sentence
        
    size_t iteration_num;
    int sentences_for_train_num;
    std::vector<Sentence *> sentences_for_train;
         
    std::vector<Node *> begin_node_list; // position -> list of nodes that begin at this pos
    std::vector<Node *> end_node_list;   // position -> list of nodes that end at this pos
    bool write_tmp_model_file(int t);
    //int online_learning(Sentence* gold, Sentence* system ); // learn と train があって紛らわしい...
    int online_learning(Sentence* gold, Sentence* system ,TopicVector *topic);
  public:
    Tagger(Parameter *in_param);
    ~Tagger();
    Sentence *new_sentence_analyze(std::string &in_sentence);
    Sentence *new_sentence_analyze_lda(std::string &in_sentence, TopicVector &topic);
    void sentence_clear();
    bool viterbi_at_position(unsigned int pos, Node *r_node);
    void print_best_path();
	void print_best_path_with_rnn(RNNLM::CRnnLM& model);
    void print_N_best_with_rnn(RNNLM::CRnnLM& model); 
    void print_N_best_path();
    void print_lattice();
    void print_old_lattice();
        
    bool train(const std::string &gsd_file);
    bool train_lda(const std::string &gsd_file, Tagger& normal_model);
    bool read_gold_data(const std::string &gsd_file);
    bool read_gold_data(const char *gsd_file);
    bool lookup_gold_data(Sentence *sentence, std::string &word_pos_pair);
    bool add_one_sentence_for_train(Sentence *in_sentence);
    void clear_gold_data();

    // write feature weights
    bool write_model_file(const std::string &model_filename) {
        std::ofstream model_out(model_filename.c_str(), std::ios::out);
        if (!model_out.is_open()) {
            cerr << ";; cannot open " << model_filename << " for writing" << endl;
            return false;
        }
        for (auto &w:weight){
            model_out << w.first << " " << w.second << endl;
        }
        model_out.close();
        return true;
    }

    // read feature weights
    bool read_model_file(const std::string &model_filename) {
        std::ifstream model_in(model_filename.c_str(), std::ios::in);
        if (!model_in.is_open()) {
            cerr << ";; cannot open " << model_filename << " for reading" << endl;
            exit(1);
        }
        std::string buffer;
        while (getline(model_in, buffer)) {
            std::vector<std::string> line;
            Morph::split_string(buffer, " ", line);
            weight[line[0]] = atof(static_cast<const char *>(line[1].c_str()));
        }
        model_in.close();
        return true;
    }

};

}

#endif

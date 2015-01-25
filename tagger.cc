#include "common.h"
#include "tagger.h"
#include "sentence.h"
#include <iterator>     // std::distance

namespace Morph {

Tagger::Tagger(Parameter *in_param) {
    param = in_param;
    ftmpl.open(param->ftmpl_filename); 

    dic.open(param, &ftmpl);

    begin_node_list.reserve(INITIAL_MAX_INPUT_LENGTH);
    end_node_list.reserve(INITIAL_MAX_INPUT_LENGTH);

    sentences_for_train_num = 0;
}

Tagger::~Tagger() {
}

// analyze a sentence
Sentence *Tagger::new_sentence_analyze(std::string &in_sentence) {
    sentence = new Sentence(&begin_node_list, &end_node_list, in_sentence, &dic, &ftmpl, param);
    sentence->lookup_and_analyze();
    return sentence;
}

// clear a sentence
void Tagger::sentence_clear() {
    delete sentence;
}

// train
bool Tagger::train(const std::string &gsd_file) {
    read_gold_data(gsd_file);

//         for iteration 1..5 {
//                 for sentence 1..N {
//                          tagger.analyzer(sentence chunked);
//                          - collected feature;
//                          feature gold standard;
//                          +
//                     }
//             }
    size_t total_iteration_num = 0;
    for (size_t t = 0; t < param->iteration_num; t++) {
        cerr << "ITERATION:" << t << endl;
        if (param->shuffle_training_data) // shuffle training data
            random_shuffle(sentences_for_train.begin(), sentences_for_train.end());
        for (std::vector<Sentence *>::iterator it = sentences_for_train.begin(); it != sentences_for_train.end(); it++) {
            cerr << std::distance(sentences_for_train.begin(),it) << "/" << std::distance(sentences_for_train.begin(), sentences_for_train.end()) << "\r";
            new_sentence_analyze((*it)->get_sentence()); // get the best path
            sentence->minus_feature_from_weight(feature_weight); // - prediction
            (*it)->get_feature()->plus_feature_from_weight(feature_weight); // + gold standard
            if (WEIGHT_AVERAGED) { // for average
                sentence->minus_feature_from_weight(feature_weight_sum, total_iteration_num); // - prediction
                (*it)->get_feature()->plus_feature_from_weight(feature_weight_sum, total_iteration_num); // + gold standard
            }
            sentence_clear();
            total_iteration_num++;
        }
        cerr << endl;
    }

    if (WEIGHT_AVERAGED) {
        for (std::unordered_map<std::string, double>::iterator it = feature_weight_sum.begin(); it != feature_weight_sum.end(); it++) {
            feature_weight[it->first] -= it->second / total_iteration_num;
        }
    }

//     for (std::vector<Sentence *>::iterator it = sentences_for_train.begin(); it != sentences_for_train.end(); it++) {
//         new_sentence_analyze((*it)->get_sentence()); // get the best path
//         print_best_path();
//         sentence_clear();
//     }

    clear_gold_data();
    return true;
}

// read gold standard data
bool Tagger::read_gold_data(const std::string &gsd_file) {
    return read_gold_data(gsd_file.c_str());
}

// read gold standard data
bool Tagger::read_gold_data(const char *gsd_file) {
    std::ifstream gsd_in(gsd_file, std::ios::in);
    if (!gsd_in.is_open()) {
        cerr << ";; cannot open gold data for reading" << endl;
        return false;
    }

    std::string buffer;
    while (getline(gsd_in, buffer)) {
        std::vector<std::string> word_pos_pairs;
        split_string(buffer, " ", word_pos_pairs);

        Sentence *new_sentence = new Sentence(strlen(buffer.c_str()), &begin_node_list, &end_node_list, &dic, &ftmpl, param);
        //cerr << buffer << endl;
        for (std::vector<std::string>::iterator it = word_pos_pairs.begin(); it != word_pos_pairs.end(); it++) {
            if(it->size()>0)//文末or文頭に空白があると空の文字列が分割に入る
                new_sentence->lookup_gold_data(*it);
        }
        new_sentence->find_best_path();
        new_sentence->set_feature();
        new_sentence->clear_nodes();
        add_one_sentence_for_train(new_sentence);
        //new_sentence->feature_print();
        sentences_for_train_num++;
    }

    gsd_in.close();
    return true;
}

// clear gold standard data
void Tagger::clear_gold_data() {
    for (std::vector<Sentence *>::iterator it = sentences_for_train.begin(); it != sentences_for_train.end(); it++) {
        delete *it;
    }
    sentences_for_train.clear();
}

// print the best path of a test sentence
void Tagger::print_best_path() {
    sentence->print_best_path();
}

void Tagger::print_N_best_path() {
    sentence->print_N_best_path();
}

void Tagger::print_lattice() {
    //sentence->print_juman_lattice();
    sentence->print_unified_lattice(); 
}

void Tagger::print_old_lattice() {
    sentence->print_juman_lattice();
}

bool Tagger::add_one_sentence_for_train (Sentence *in_sentence) {
    sentences_for_train.push_back(in_sentence);
    return true;
}

}

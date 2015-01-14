#ifndef PARAMETER_H
#define PARAMETER_H

#include "common.h"

namespace Morph {

class Parameter {
  public:
    bool unknown_word_detection;
    bool shuffle_training_data;
    bool debug;
    bool nbest;
    unsigned int unk_max_length;
    unsigned int iteration_num;
    std::string darts_filename;
    std::string dic_filename;
    std::string pos_filename;
	std::string spos_filename;
	std::string form_filename;
	std::string form_type_filename;
	std::string base_filename;
    std::string ftmpl_filename;
    std::string rep_filename;
    std::string imis_filename;
    std::string reading_filename;
	unsigned int N = 5;
	unsigned int N_redundant;

    std::vector<unsigned short> unk_pos;
    std::vector<unsigned short> unk_figure_pos;

    Parameter(const std::string &in_dic_filename, const std::string &in_ftmpl_filename, const int in_iteration_num, const bool in_unknown_word_detection, const bool in_shuffle_training_data, const unsigned int in_unk_max_length, const bool in_debug, const bool in_nbest) {
        darts_filename = in_dic_filename + ".da";
        dic_filename = in_dic_filename + ".bin";
        pos_filename = in_dic_filename + ".pos" ;
		spos_filename = in_dic_filename + ".spos";
		form_filename = in_dic_filename  + ".form";
		form_type_filename = in_dic_filename + ".form_type";
		base_filename = in_dic_filename + ".base";
        ftmpl_filename = in_ftmpl_filename;
        rep_filename = in_dic_filename + ".rep";
        imis_filename = in_dic_filename + ".imis";
        reading_filename = in_dic_filename + ".read";
        iteration_num = in_iteration_num;
        unknown_word_detection = in_unknown_word_detection;
        shuffle_training_data = in_shuffle_training_data;
        unk_max_length = in_unk_max_length;
        debug = in_debug;
        nbest = in_nbest;

		N_redundant = N + (unsigned int)(ceil(N * B_BEST_REDUNDANCY));
    }
};

}

#endif

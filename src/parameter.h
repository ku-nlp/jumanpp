#ifndef PARAMETER_H
#define PARAMETER_H

#include "common.h"

namespace Morph {

class Parameter {
    friend int main(int argc, char **argv);

  public:
    bool unknown_word_detection;
    bool shuffle_training_data;
    bool debug = false;
    bool rnndebug = false;
    bool nbest;
    bool beam;
    bool use_so;
    bool trigram = true;
    bool rnnlm = false;
    bool print_gold = false;
    bool nce = false;
    bool userep = false;
    bool usebase = false;
    bool usesurf = false;
    bool usepos = true;
    bool srilm;
    bool useoldloss = false;
    bool usetypedloss = false;
    bool lpenalty = false;
    bool no_posmatch = true;
    bool use_suu_rule = true;
    bool use_lexical_feature = false;
    bool use_rnnlm_as_feature = false;
    bool use_dynamic_loading = false;
    double rweight;
    double lweight;
    unsigned int unk_max_length;
    unsigned int iteration_num;
    std::string delimiter;
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
    std::string model_filename;
    std::string freq_word_list;
    unsigned int N = 1;
    unsigned int L = 1;
    bool use_scw = false;
    bool passive_unknown = false;
    unsigned int N_redundant; //もう使っていない
    bool output_ambiguous_word = false;
    double c_value = 1.0;
    double phi_value = 1.0;
    bool use_total_sim = false;

    std::vector<unsigned long> unk_pos;
    std::vector<unsigned long> unk_figure_pos;

    int set_N(unsigned int n) {
        if (n > 1000)
            n = 1000;
        if (n == 0)
            n = 1;
        N = n;
        N_redundant = N + (unsigned int)(ceil(N * B_BEST_REDUNDANCY));
        return 0;
    }
    int set_L(unsigned int l) {
        if (l > 1000)
            l = 1000;
        if (l == 0)
            l = 1;
        L = l;
        return 0;
    }

    int set_nbest(bool in_nb) {
        nbest = in_nb;
        return 0;
    }
    int set_passive_unknown(bool in_pas_unk) {
        passive_unknown = in_pas_unk;
        return 0;
    }
    int set_beam(bool in_beam) {
        beam = in_beam;
        return 0;
    }
    int set_rnnlm(bool in_rnnlm) {
        rnnlm = in_rnnlm;
        return 0;
    }
    int set_nce(bool in_nce) {
        nce = in_nce;
        return 0;
    }
    int set_srilm(bool in_srilm) {
        srilm = in_srilm;
        return 0;
    }
    int set_rweight(double rvalue) {
        rweight = rvalue;
        return 0;
    }
    int set_lpenalty(bool lp) {
        lpenalty = lp;
        return 0;
    }
    int set_lweight(double lvalue) {
        lpenalty = true;
        lweight = lvalue;
        return 0;
    }

    void set_output_ambigous_word(bool b) { output_ambiguous_word = b; }
    void set_use_scw(bool scw) { use_scw = scw; }
    void set_so(bool in_so) { use_so = in_so; }
    void set_trigram(bool tri) { trigram = tri; }
    void set_C(double in_c) { c_value = in_c; }
    void set_Phi(double in_phi) { phi_value = in_phi; }
    void set_use_total_sim() { use_total_sim = true; }
    void set_model_filename(std::string filename) { model_filename = filename; }

    Parameter(const std::string &in_dic_filename,
              const std::string &in_ftmpl_filename, const int in_iteration_num,
              const bool in_unknown_word_detection,
              const bool in_shuffle_training_data,
              const unsigned int in_unk_max_length, const bool in_debug,
              const bool in_nbest) {
        darts_filename = in_dic_filename + ".da";
        dic_filename = in_dic_filename + ".bin";
        pos_filename = in_dic_filename + ".pos";
        spos_filename = in_dic_filename + ".spos";
        form_filename = in_dic_filename + ".form";
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

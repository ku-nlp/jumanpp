#ifndef SENTENCE_H
#define SENTENCE_H

#include "common.h"

#include "feature.h"
#include "dic.h"
#include "parameter.h"
#include "charlattice.h"
#include <math.h>
#include <memory>
#include "rnnlm/rnnlmlib.h"

// namespace SRILM {
#ifdef USE_SRILM
#include "srilm/Ngram.h"
#include "srilm/Vocab.h"
#endif

//}

namespace Morph {

// TODO: Sentence クラスが肥大化しすぎている．

class Sentence { //{{{
    Parameter *param;
    Dic *dic;
    FeatureTemplateSet *ftmpl;
    bool unknown_word_detection;
    std::vector<Node> gold_morphs; //正例の場合のみ利用

    size_t word_num;
    unsigned int length;                       // length of this sentence
    std::string sentence;                      // Gold用だが名前が悪い
    std::string comment;                       // 主にGold用
    std::string partically_annotated_sentence; // 部分アノテーション付き入力文
    std::vector<unsigned int> maxlen_for_charnum;
    const char *sentence_c_str;
    FeatureSet *feature;
    std::vector<Node *>
        *begin_node_list; // position -> list of nodes that begin at this pos
    std::vector<Node *>
        *end_node_list; // position -> list of nodes that end at this pos
    unsigned int reached_pos;
    unsigned int reached_pos_of_pseudo_nodes;

    unsigned int first_delimiter;
    unsigned int last_delimiter;
    std::vector<unsigned int> delimiter_position;
    // std::vector<unsigned int> delimiter_interval;
    bool output_ambiguous_word;
    std::unique_ptr<FeatureSet> beam_feature;

    static std::shared_ptr<RNNLM::context> initial_context;
    static RNNLM::CRnnLM *rnnlm;
#ifdef USE_SRILM
    static Ngram *srilm;
    static Vocab *vocab;
#endif
    explicit Sentence(const Sentence &s){};

  public:
    static void init_rnnlm(RNNLM::CRnnLM *model);
    static void init_rnnlm_FR(RNNLM::CRnnLM *model);
#ifdef USE_SRILM
    static void init_srilm(Ngram *model, Vocab *vocab);
#endif
    Sentence(std::vector<Node *> *in_begin_node_list,
             std::vector<Node *> *in_end_node_list, std::string &in_sentence,
             Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    Sentence(std::vector<Node *> *in_begin_node_list,
             std::vector<Node *> *in_end_node_list, std::string &in_sentence,
             Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param,
             std::string delimiter);
    Sentence(size_t max_byte_length, std::vector<Node *> *in_begin_node_list,
             std::vector<Node *> *in_end_node_list, Dic *in_dic,
             FeatureTemplateSet *in_ftmpl, Parameter *in_param);

    Sentence(std::vector<Node *> *in_begin_node_list,
             std::vector<Node *> *in_end_node_list, std::string &in_sentence,
             Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param,
             unsigned int format_type);

    void init(size_t max_byte_length, std::vector<Node *> *in_begin_node_list,
              std::vector<Node *> *in_end_node_list, Dic *in_dic,
              FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    ~Sentence();
    void clear_nodes();

    // beam の場合はhistory をたどるように変更
    void set_gold_nodes_beam() { //{{{
        auto &eos_beam = (*begin_node_list)[length]->bq.beam;

        if (eos_beam.size() == 0) {
            cerr << ";; gold parse err" << endl;
            return;
        }

        auto &eos_tok = eos_beam.back();
        std::vector<Node *> history = eos_tok.node_history;

        bool find_bos_node_gold = false;
        for (auto node_gold = history.rbegin(); node_gold != history.rend();
             node_gold++) {
            if ((*node_gold)->stat == MORPH_BOS_NODE)
                find_bos_node_gold = true;
            Node new_node;
            new_node.stat = (*node_gold)->stat;
            new_node.posid = (*node_gold)->posid;
            new_node.pos = (*node_gold)->pos;
            new_node.sposid = (*node_gold)->sposid;
            new_node.spos = (*node_gold)->spos;
            new_node.form = (*node_gold)->form;
            new_node.formid = (*node_gold)->formid;
            new_node.form_type = (*node_gold)->form_type;
            new_node.formtypeid = (*node_gold)->formtypeid;
            new_node.imisid = (*node_gold)->imisid;
            new_node.semantic_feature = (*node_gold)->semantic_feature;

            new_node.base = (*node_gold)->base;
            new_node.representation = (*node_gold)->representation;
            new_node.length = (*node_gold)->length;
            new_node.char_num = (*node_gold)->char_num;
            gold_morphs.push_back(new_node);

            // debug 用出力
            // cerr << *new_node.base << "_"<< *new_node.pos << ":" <<
            // *new_node.spos << " ";
        }
        // debug 用
        // cerr << endl;

        if (!find_bos_node_gold)
            cerr << ";; gold parse err" << endl;
    } //}}}

    void set_gold_nodes() { //{{{
        Node *node_gold = (*begin_node_list)[length];
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

            // new_node.surface = node_gold->surface; //無意味
            // new_node.string_for_print = node_gold->string_for_print; //エラー
            new_node.base = node_gold->base;
            new_node.representation = node_gold->representation;
            new_node.length = node_gold->length;
            new_node.char_num = node_gold->char_num;
            gold_morphs.push_back(new_node);
            node_gold = node_gold->prev;
        }
        if (!find_bos_node_gold)
            cerr << ";; gold parse err" << endl;
    } //}}}

    void print_gold_beam();

    std::vector<std::string> get_gold_topic_features(TopicVector *tov);

    bool lookup_and_analyze();
    bool lookup_and_analyze_partial();
    bool lookup();
    bool lookup_partial();
    bool analyze();
    bool add_one_word(std::string &word);
    void set_comment(std::string com_str) { /*{{{*/
        comment = com_str;
    }                            /*}}}*/
    std::string &get_comment() { /*{{{*/
        return comment;
    } /*}}}*/
    std::string &get_sentence() { return sentence; }
    unsigned int get_length() { return length; }
    std::string &get_pa_sentence() { return partically_annotated_sentence; }

    // gold でしか使わない
    FeatureSet *get_feature() { //{{{
        if (beam_feature)
            return beam_feature.get();
        else
            return feature;
    } //}}}

    // 学習用に探索結果に素性ベクトルを再設定
    void generate_beam_feature() { //{{{
        if (beam_feature)
            return;
        if (feature != nullptr)
            return;

        std::unique_ptr<FeatureSet> fs(new FeatureSet(ftmpl));
        auto token = (*begin_node_list)[length]->bq.beam.front();
        std::vector<Node *> result_morphs = token.node_history;
        Node *last_node = nullptr;
        Node *last_last_node = nullptr;
        for (auto n_ptr : result_morphs) {
            fs->extract_unigram_feature(n_ptr);
            if (last_node)
                fs->extract_bigram_feature(last_node, n_ptr);
            if (last_last_node) {
                fs->extract_trigram_feature(last_last_node, last_node, n_ptr);
            }
            last_last_node = last_node; // node_before_last
            last_node = n_ptr;
        }
        beam_feature = std::move(fs);
        return;
    } //}}}

    FeatureSet &get_best_feature() { //{{{
        generate_beam_feature();
        return *beam_feature; //参照を返す
    }                         //}}}

    // ベストのパスのfeature を 文のfeature に移す
    FeatureSet *set_feature() { //{{{
        if (feature) {
            delete feature;
            feature = nullptr;
        }
        feature = new FeatureSet(*(*begin_node_list)[length]->feature); //コピー
        //(*begin_node_list)[length]->feature = nullptr;
        return feature;
    } //}}}

    bool set_feature_beam() { //{{{
        if (feature) {        // beam では feature は使わない
            delete feature;
            feature = nullptr;
        }
        generate_beam_feature();
        // feature = beam_feature.release();
        // //((*begin_node_list)[length]->bq.beam.front().f.release());
        return true;
    } //}}}

    void feature_print();

    bool check_devoice_condition(
        Node &node) { //次の形態素が濁音化できるかどうかのチェック//{{{
                      /* 濁音化するのは直前の形態素が名詞、または動詞の連用形、名詞性接尾辞の場合のみ
                       */
        /* 接尾辞の場合を除き直前の形態素が平仮名1文字となるものは不可 */
        /* 連濁は文の先頭は不可 */

        bool check_verb =
            (node.posid == dic->posid2pos.get_id("動詞") &&
             (node.formid == dic->formid2form.get_id("基本連用形") ||
              node.formid ==
                  dic->formid2form.get_id(
                      "タ接連用形"))); // 動詞で連用形. Juman
        // の時点で，連用形は母音動詞の連用形と同じid
        // かどうかだけをチェックしている
        bool check_noun =
            (node.posid == dic->posid2pos.get_id("名詞") &&
             node.sposid !=
                 dic->sposid2spos.get_id("形式名詞")); // 形式名詞以外の名詞
        bool check_postp =
            (node.posid == dic->posid2pos.get_id("接尾辞") &&
             (node.sposid == dic->sposid2spos.get_id("名詞性述語接尾辞") ||
              node.sposid == dic->sposid2spos.get_id("名詞性名詞接尾辞") ||
              node.sposid == dic->sposid2spos.get_id("名詞性名詞助数辞") ||
              node.sposid ==
                  dic->sposid2spos.get_id("名詞性特殊接尾辞"))); //接尾辞
        bool check_hiragana =
            (!(node.posid != dic->posid2pos.get_id("接尾辞") &&
               node.char_type == TYPE_HIRAGANA &&
               node.char_num == 1)); //接尾辞以外のひらがな一文字でない
        bool check_bos = !(node.stat & MORPH_BOS_NODE); //文の先頭でない
        ///* カタカナ複合語の連濁は認めない */ //未実装

        return (check_bos & check_hiragana &
                (check_verb | check_noun | check_postp));
    }; //}}}

    Node *get_bos_node();
    Node *get_eos_node();
    void set_begin_node_list(unsigned int pos, Node *new_node);
    void set_end_node_list(unsigned int pos, Node *r_node);

    bool beam_at_position(unsigned int pos, Node *r_node);
    unsigned int find_reached_pos(unsigned int pos, Node *node);
    unsigned int find_reached_pos_of_pseudo_nodes(unsigned int pos, Node *node);
    void print_beam();
    void print_best_beam();
    void print_best_beam_rep();

    void generate_unified_lattice_line(Node *node, std::stringstream &ss,
                                       std::vector<std::vector<int>> &num2id,
                                       unsigned int char_num);
    void generate_juman_line(Node *node,
                             std::stringstream &output_string_buffer,
                             std::string prefix = "");
    void print_best_beam_juman();
    void print_best_path_with_rnn(RNNLM::CRnnLM &model);
    // TopicVector get_topic();
    std::map<std::string, int> nbest_duplicate_filter;
    void mark_nbest();
    void mark_nbest_rbeam(unsigned int nbest);
    Node *find_best_beam();
    double eval(Sentence &gold);
    void print_unified_lattice();
    void print_unified_lattice_rbeam(unsigned int nbest);
    bool lookup_gold_data(std::string &word_pos_pair);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str,
                                               unsigned int pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str,
                                               unsigned int specified_length,
                                               std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str,
                                               unsigned int pos,
                                               unsigned int specified_length,
                                               std::string *specified_pos);
    Node *
    lookup_and_make_special_pseudo_nodes(const char *start_str,
                                         unsigned int specified_length,
                                         const std::vector<std::string> &spec);
    Node *lookup_and_make_special_pseudo_nodes_lattice(
        CharLattice &cl, const char *start_str, unsigned int char_num,
        unsigned int pos, unsigned int specified_length,
        std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes_lattice(
        CharLattice &cl, const char *start_str, unsigned int specified_length,
        const std::vector<std::string> &spec); // spec 版

    Node *lookup_and_make_special_pseudo_nodes_lattice_with_max_length(
        CharLattice &cl, const char *start_str, unsigned int char_num,
        unsigned int pos, unsigned int specified_length,
        std::string *specified_pos, unsigned int max_length);
    bool check_dict_match(Node *tmp_node, Node *dic_node);

    Node *
    make_unk_pseudo_node_list_from_previous_position(const char *start_str,
                                                     unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_from_some_positions(
        const char *start_str, unsigned int pos, unsigned int previous_pos,
        unsigned int max_length = 0);
    Node *
    make_unk_pseudo_node_list_by_dic_check(const char *start_str,
                                           unsigned int pos, Node *r_node,
                                           unsigned int specified_char_num);

    // 最大長を超えるノードを orig_dic から削除し，ポインタを返す
    Node *filter_long_node(Node *orig_dic, unsigned int max_length);

    // TODO::処理の共通化
    std::string generate_rnnlm_representation(Node *r_node) {
        std::string rnnlm_rep;
        if (*r_node->spos == UNK_POS ||
            *r_node->spos ==
                "数詞") { //未定義語，数詞は表層から擬似代表表記を作る
            rnnlm_rep =
                *(r_node->original_surface) + "/" + *(r_node->original_surface);
        } else if (r_node->representation == nullptr ||
                   *r_node->representation == "*" ||
                   *r_node->representation == "<UNK>" ||
                   *r_node->representation == "") {
            // 代表表記の無い語は,
            // 読みの有無にかかわらず，原形+原形で擬似代表表記を作成
            // ０の読みなど、対処の難しい不一致を回避するため。
            rnnlm_rep = *r_node->base + "/" + *r_node->base;
        } else {
            rnnlm_rep = *r_node->representation;
        }
        // std::cerr << rnnlm_rep << " " ;
        return rnnlm_rep;
    }

    std::string generate_rnnlm_token(Node *r_node) { /*{{{*/
        std::string rnnlm_rep;
        if (param->userep) {
            rnnlm_rep = generate_rnnlm_representation(r_node);
        } else if (param->usesurf) {
            rnnlm_rep = *(r_node->original_surface);
        } else { // usebase // 原形用言語モデル
            rnnlm_rep = (*r_node->spos == UNK_POS || *r_node->spos == "数詞")
                            ? *(r_node->original_surface)
                            : *(r_node->base);
        }

        if (param->usepos) {
            // 未定義語の場合もここでは推定された品詞が入っている。
            rnnlm_rep += "_" + *r_node->pos;
        }
        return rnnlm_rep;
    } /*}}}*/

    size_t get_rnnlm_word_length(Node *r_node) { /*{{{*/
        if (*r_node->spos == UNK_POS || *r_node->spos == "数詞")
            return U8string::character_length(*(r_node->original_surface));
        else
            return U8string::character_length(*(r_node->base));
    } /*}}}*/
  private:
    void generate_imis(Node *node, std::stringstream &output_string_buffer,
                       std::string quot);

}; //}}}
}

#endif

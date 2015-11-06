#ifndef SENTENCE_H
#define SENTENCE_H

#include "common.h"

#include "feature.h"
#include "dic.h"
#include "parameter.h"
#include "charlattice.h"
#include "u8string.h"
#include <math.h>
#include <memory>
#include <boost/tr1/unordered_map.hpp>
#include "rnnlm/rnnlmlib.h"

//namespace SRILM {
#ifdef USE_SRILM
#include "srilm/Ngram.h"
#include "srilm/Vocab.h"
#endif

//}

namespace Morph {

class Sentence {//{{{
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
    std::unique_ptr<FeatureSet> beam_feature;
            
    static std::shared_ptr<RNNLM::context> initial_context; 
    static RNNLM::CRnnLM* rnnlm;
#ifdef USE_SRILM
    static Ngram*  srilm;
    static Vocab*  vocab;
#endif
    explicit Sentence(const Sentence &s){ };
  public:
    static void init_rnnlm(RNNLM::CRnnLM* model);
    static void init_rnnlm_FR(RNNLM::CRnnLM* model);
#ifdef USE_SRILM
    static void init_srilm(Ngram* model, Vocab* vocab);
#endif
    Sentence(std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, std::string &in_sentence, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    Sentence(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    void init(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param);
    ~Sentence();
    void clear_nodes();

    // beam の場合はhistory をたどるように変更
    void set_gold_nodes_beam(){//{{{
        auto& eos_beam = (*begin_node_list)[length]->bq.beam;

        if(eos_beam.size() == 0){
            cerr << ";; gold parse err"<< endl;
            return;
        }

        auto& eos_tok = eos_beam.back(); 
        std::vector<Node *> history = eos_tok.node_history;

        bool find_bos_node_gold = false;
		for (auto node_gold = history.rbegin(); node_gold != history.rend(); node_gold++) {
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
            //cerr << *new_node.base << "_"<< *new_node.pos << ":" << *new_node.spos << " ";
        }
        //debug 用
        //cerr << endl;

        if(!find_bos_node_gold)
            cerr << ";; gold parse err"<< endl;
    }//}}}

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

    // gold でしか使わない
    FeatureSet *get_feature() {//{{{
        if(beam_feature)
            return beam_feature.get();
        else
            return feature;
    }//}}}

    void  generate_beam_feature(){//{{{
        if(beam_feature) return;
        if(feature != nullptr) return;
            
        std::unique_ptr<FeatureSet> fs(new FeatureSet(ftmpl));
        auto token = (*begin_node_list)[length]->bq.beam.front();
        std::vector<Node *> result_morphs = token.node_history;
        Node* last_node = nullptr;
        Node* last_last_node = nullptr;
        for(auto n_ptr:result_morphs){
            fs->extract_unigram_feature(n_ptr);
            if(last_node)
                fs->extract_bigram_feature(last_node,n_ptr);
            if(last_last_node)
                fs->extract_trigram_feature(last_last_node, last_node, n_ptr);
            last_last_node = last_node;
            last_node = n_ptr;
        }
        beam_feature = std::move(fs); 
        return;
    }//}}}

    // ベストのパスのfeature を 文のfeature に移す
    FeatureSet *set_feature() {//{{{
        if (feature){
            delete feature;
            feature =nullptr;
        }
        feature = new FeatureSet(*(*begin_node_list)[length]->feature); //コピー
        //(*begin_node_list)[length]->feature = nullptr;
        return feature;
    }//}}}
    
    bool set_feature_beam(){//{{{
        if (feature){ //beam では feature は使わない
            delete feature;
            feature = nullptr;
        }
        generate_beam_feature();
        //feature = beam_feature.release(); //((*begin_node_list)[length]->bq.beam.front().f.release());
        return true;
    }//}}}

    FeatureSet *set_feature_so() {//{{{
        if (feature)
            delete feature;
        feature = new FeatureSet(*(*begin_node_list)[length + 1]->get_bigram_feature((*begin_node_list)[length])); //コピー
        //(*begin_node_list)[length + 1]->set_bigram_feature((*begin_node_list)[length], NULL);
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
        //if(node->bq.beam.size()> 0)
        if(param->beam){
            generate_beam_feature();
            return *beam_feature; //参照を返す
            //return *node->bq.beam.front().f; //参照を返す
        }else if(param->use_so)
            return (*(*begin_node_list)[length + 1]->get_bigram_feature((*begin_node_list)[length]));
        else
            return *node->feature; // beam 探索していない場合
    }//}}}
        
    Node *get_bos_node();
    Node *get_eos_node();
    Node *find_best_path();
    void set_begin_node_list(unsigned int pos, Node *new_node);
    void set_end_node_list(unsigned int pos, Node *r_node);

    bool so_viterbi_at_position(unsigned int pos, Node *r_node);
    //so_viterbi_at_position_nbest(pos, (*begin_node_list)[pos]);
    bool viterbi_at_position(unsigned int pos, Node *r_node);
	bool viterbi_at_position_nbest(unsigned int pos, Node *r_node);
	bool beam_at_position(unsigned int pos, Node *r_node);
    //bool beam_at_position(unsigned int pos, Node *r_node, std::vector<BeamQue> &bq );
    unsigned int find_reached_pos(unsigned int pos, Node *node);
    unsigned int find_reached_pos_of_pseudo_nodes(unsigned int pos, Node *node);
    void print_beam();
    void print_best_beam();
    void print_best_path();

    void generate_juman_line(Node* node, std::stringstream &output_string_buffer, std::string prefix = "");
    void print_best_beam_juman();
    void print_best_path_with_rnn(RNNLM::CRnnLM& model);
    TopicVector get_topic();
    void print_lattice();
	std::map<std::string, int> nbest_duplicate_filter;
	void print_N_best_path();
	void print_N_best_with_rnn(RNNLM::CRnnLM& model);
    void mark_nbest();
    void mark_nbest_rbeam(unsigned int nbest); 
    Node* find_N_best_path(); // make EOS node and get N-best path
    Node *find_best_beam();
    double eval(Sentence& gold);
    void print_juman_lattice(); // 互換性の為
    void print_unified_lattice(); 
    void print_unified_lattice_rbeam(unsigned int nbest); 
    void minus_feature_from_weight(Umap &in_feature_weight);
    void minus_feature_from_weight(Umap &in_feature_weight, size_t factor);
    bool lookup_gold_data(std::string &word_pos_pair);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int specified_length, const std::vector<std::string>& spec);
    Node *lookup_and_make_special_pseudo_nodes_lattice(CharLattice &cl, const char *start_str, unsigned int char_num, unsigned int pos, unsigned int specified_length, std::string *specified_pos); 
    Node *lookup_and_make_special_pseudo_nodes_lattice(CharLattice &cl, const char *start_str, unsigned int specified_length, const std::vector<std::string>& spec); //spec 版
    bool check_dict_match(Node* tmp_node, Node* dic_node);
    
    Node *make_unk_pseudo_node_list_from_previous_position(const char *start_str, unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_from_some_positions(const char *start_str, unsigned int pos, unsigned int previous_pos);
    Node *make_unk_pseudo_node_list_by_dic_check(const char *start_str, unsigned int pos, Node *r_node, unsigned int specified_char_num);

    Node *find_best_path_so() {//{{{

        (*begin_node_list)[length] = get_eos_node(); // End Of Sentence

        set_bigram_info(length, (*begin_node_list)[length]);
        so_viterbi_at_position(length, (*begin_node_list)[length ]);
        (*end_node_list)[length + 1] = (*begin_node_list)[length]; //EOS ノード

        set_end_node_list(length, (*begin_node_list)[length + 1]);
        (*begin_node_list)[length + 1] = get_eos_node(); // End Of Sentence
        set_bigram_info(length + 1, (*begin_node_list)[length + 1]);
        so_viterbi_at_position(length + 1, (*begin_node_list)[length + 1]);

        return (*begin_node_list)[length + 1];
    }//}}}
    
    // Second order viterbi 用
    void set_bigram_info(unsigned int pos, Node *r_node) {//{{{
        while (r_node) {
            if( r_node->init_bigram_info == true){
                r_node = r_node->bnext;
                continue;
            }
            r_node->reserve_best_bigram();
            Node *l_node = (*end_node_list)[pos];
            while (l_node) {
                // r_node->best_bigram.insert(std::map<unsigned int, BigramInfo *>::value_type(l_node->id, new BigramInfo()));
                r_node->best_bigram.insert(std::make_pair(l_node->id, new BigramInfo()));
                // r_node->best_bigram[l_node->id] = new BigramInfo();
                r_node->best_bigram[l_node->id]->feature = new FeatureSet(ftmpl);
                r_node->best_bigram[l_node->id]->feature->extract_bigram_feature(l_node, r_node);
                r_node->best_bigram[l_node->id]->cost = r_node->best_bigram[l_node->id]->feature->calc_inner_product_with_weight();
                r_node->best_bigram[l_node->id]->cost += l_node->wcost;
                r_node->best_bigram[l_node->id]->feature->append_feature(l_node->feature); // unigram feature of l_node
                r_node->best_bigram[l_node->id]->total_cost = r_node->best_bigram[l_node->id]->cost;
                l_node = l_node->enext;
            }
            r_node->init_bigram_info = true;
            r_node = r_node->bnext;
        }
    }//}}}

};//}}}



}

#endif

#include "common.h"
#include "pos.h"
#include "sentence.h"
#include <utility>
#include <algorithm>
#include <memory>
//#include "feature.h"

// namespace SRILM {
//#include "srilm/Ngram.h"
//}

#include <sstream>

// SRILM memo
//
// #ifdef USE_SHORT_VOCAB
// typedef unsigned short	VocabIndex;
// #else
// typedef unsigned int	VocabIndex;
// #endif
//
// typedef const char	*VocabString;
// * VocabIndex getIndex(VocabString token)
// *	Returns the index for a string token, or Vocab_None if none exists.
//
// LogP wordProb(VocabIndex word, const VocabIndex *context);
// * returns word probability
// OOV オプションが付いている場合は，OOV

namespace Morph {

std::string BOS_STRING = BOS;
std::string EOS_STRING = EOS;
std::shared_ptr<RNNLM::context> Sentence::initial_context;
RNNLM::CRnnLM *Sentence::rnnlm;
Ngram *Sentence::srilm;
Vocab *Sentence::vocab;

void Sentence::init_rnnlm(RNNLM::CRnnLM *in_rnnlm) {
    rnnlm = in_rnnlm;
    RNNLM::context tmp;
    rnnlm->get_initial_context(&tmp);
    initial_context = std::make_shared<RNNLM::context>(tmp);
}
void Sentence::init_srilm(Ngram *model, Vocab *in_vocab) {
    srand48((long)1);
    srilm = model;
    vocab = in_vocab;
}

// for test sentence
Sentence::Sentence(std::vector<Node *> *in_begin_node_list,
                   std::vector<Node *> *in_end_node_list,
                   std::string &in_sentence, Dic *in_dic,
                   FeatureTemplateSet *in_ftmpl, Parameter *in_param) {  //{{{
    sentence_c_str = in_sentence.c_str();
    length = strlen(sentence_c_str);
    init(length, in_begin_node_list, in_end_node_list, in_dic, in_ftmpl,
         in_param);
}  //}}}

// for gold sentence
Sentence::Sentence(size_t max_byte_length,
                   std::vector<Node *> *in_begin_node_list,
                   std::vector<Node *> *in_end_node_list, Dic *in_dic,
                   FeatureTemplateSet *in_ftmpl, Parameter *in_param) {  //{{{
    length = 0;

    init(max_byte_length, in_begin_node_list, in_end_node_list, in_dic,
         in_ftmpl, in_param);
}  //}}}

void Sentence::init(size_t max_byte_length,
                    std::vector<Node *> *in_begin_node_list,
                    std::vector<Node *> *in_end_node_list, Dic *in_dic,
                    FeatureTemplateSet *in_ftmpl, Parameter *in_param) {  //{{{
    param = in_param;
    dic = in_dic;
    ftmpl = in_ftmpl;
    word_num = 0;
    feature = NULL;
    reached_pos = 0;
    reached_pos_of_pseudo_nodes = 0;

    begin_node_list = in_begin_node_list;
    end_node_list = in_end_node_list;
    output_ambiguous_word = in_param->output_ambiguous_word;

    if (begin_node_list->capacity() <= max_byte_length) {
        begin_node_list->reserve(max_byte_length + 1);
        end_node_list->reserve(max_byte_length + 1);
    }

    begin_node_list->clear();
    begin_node_list->resize(max_byte_length + 4, nullptr);
    end_node_list->clear();
    end_node_list->resize(max_byte_length + 4, nullptr);
    (*end_node_list)[0] = get_bos_node();  // Begin Of Sentence
}  //}}}

Sentence::~Sentence() {  //{{{
    if (feature) {
        delete feature;
        feature = nullptr;
    }
    clear_nodes();
}  //}}}

void Sentence::clear_nodes() {  //{{{
    // EOS 以外基本的にbegin_node_list と同じノードを指しているはず
    if (end_node_list && end_node_list->size() > 0 && (*end_node_list)[0]) {
        delete (*end_node_list)[0];  // delete BOS
        (*end_node_list)[0] = nullptr;
    } else {
        // std::cerr << "skipped: " << this->sentence << " clear_end_nodes" <<
        // std::endl;
    }

    if (begin_node_list && begin_node_list->size() > 0) {
        for (unsigned int pos = 0; pos <= length; pos++) {
            Node *tmp_node = (*begin_node_list)[pos];
            while (tmp_node) {
                Node *next_node = tmp_node->bnext;
                delete tmp_node;
                tmp_node = next_node;
            }
        }
    } else {
        // std::cerr << "skipped: " << this->sentence << " clear_begin_nodes" <<
        // std::endl;
    }

    if (begin_node_list) begin_node_list->clear();
    if (end_node_list) end_node_list->clear();
}  //}}}

bool Sentence::add_one_word(std::string &word) {  //コーパス読み込み時に使用 {{{
    word_num++;
    length += strlen(word.c_str());
    sentence += word;
    return true;
}  //}}}

void Sentence::feature_print() {  //{{{
    feature->print();
}  //}}}

// make unknown word candidates of specified length if it's not found in dic
Node *Sentence::make_unk_pseudo_node_list_by_dic_check(const char *start_str, unsigned int pos, Node *r_node, unsigned int specified_char_num) {//{{{

    auto result_node = dic->make_unk_pseudo_node_list_some_pos_by_dic_check(start_str + pos, specified_char_num, MORPH_DUMMY_POS, &(param->unk_pos), r_node);

    if(result_node){
        auto tmp_node = result_node;
        while (tmp_node->bnext) {
            tmp_node = tmp_node->bnext;
            std::cout << tmp_node->string_for_print << std::endl;
        }
        tmp_node->bnext = r_node;
        return result_node;
    }else
        return r_node;
    

//    bool find_this_length = false;
//    Node *tmp_node = r_node;
//    while (tmp_node) {
//        if (tmp_node->char_num == specified_char_num) {
//            find_this_length = true;
//            break;
//        }
//        tmp_node = tmp_node->bnext;
//    }
//
//    if (!find_this_length) {  // if a node of this length is not found in dic
//        Node *result_node = dic->make_pseudo_node_list_in_range( start_str + pos, specified_char_num, specified_char_num);
//        if (result_node) {
//            if (r_node) {
//                tmp_node = result_node;
//                while (tmp_node->bnext) tmp_node = tmp_node->bnext;
//                tmp_node->bnext = r_node;
//            }
//            return result_node;
//        }
//    }
//    return r_node;
}  //}}}

// 現在地までの範囲を未定義語として生成 (任意の単語数の未定義語を生成する)
Node *Sentence::make_unk_pseudo_node_list_from_previous_position(const char *start_str, unsigned int previous_pos) {  //{{{
    if ((*end_node_list)[previous_pos] != NULL) {
        Node **node_p = &((*begin_node_list)[previous_pos]);
        while (*node_p) {
            node_p = &((*node_p)->bnext);
        }
        *node_p = dic->make_pseudo_node_list_in_range(start_str + previous_pos, 2, param->unk_max_length);
        find_reached_pos(previous_pos, *node_p);
        set_end_node_list(previous_pos, *node_p);
        return *node_p;
    } else {
        return NULL;
    }
}  //}}}

// 特定位置からの未定義語を生成 (解析中にパスが存在しなくなった時に,
// 出来なくなった時点から未定義語扱いにしてパスをつなげる)
Node *Sentence::make_unk_pseudo_node_list_from_some_positions(const char *start_str, unsigned int pos, unsigned int previous_pos) {  //{{{
    Node *node;
    node = dic->make_pseudo_node_list_in_range(start_str + pos, 1, param->unk_max_length);
    set_begin_node_list(pos, node);
    find_reached_pos(pos, node);

    // make unknown words from the prevous position
    // if (pos > 0)
    //    make_unk_pseudo_node_list_from_previous_position(start_str,
    //    previous_pos);

    return node;
}  //}}}

// specified spec 版 //文字列全体が単語候補の場合のみ
Node *Sentence::lookup_and_make_special_pseudo_nodes_lattice(CharLattice &cl, const char *start_str, unsigned int specified_length, const std::vector<std::string> &spec) {  //{{{
    Node *result_node = NULL;
    unsigned int char_num = 0;
    // Node *kanji_result_node = NULL;
    unsigned int pos = 0;
    // spec (0: reading, 1:base, 2:pos, 3:spos, 4:type, 5:form)
    auto specified_pos_string = spec[2];  // pos
    auto specified_pos = &specified_pos_string;

    // まず探す
    // std::cerr << start_str << std::endl;
    auto lattice_result = cl.da_search_from_position(dic->darts, char_num);  // こっちは何文字目かが必要
    // 以下は何バイト目かが必要
    // look up a dictionary with common prefix search
    // Node *dic_node = dic->lookup_lattice(lattice_result, start_str + pos,
    // specified_length, specified_pos);

    Node *dic_node = dic->lookup_lattice_specified(lattice_result, start_str, specified_length, spec);

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
    // 同じ文字種が続く範囲を一語として入れてくれる
    if (specified_length) {
        // 数詞処理
        result_node = dic->make_specified_pseudo_node_by_dic_check(
            start_str + pos, specified_length, specified_pos,
            &(param->unk_figure_pos), TYPE_FAMILY_FIGURE, nullptr);

        // alphabet
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_FAMILY_ALPH_PUNC);//r_node 指定してない
        }

        // 漢字

        // カタカナ
        // TODO:切れるところまでで未定義語を作る
        if (!result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_KATAKANA, nullptr); //ここも
        }
    }

    // 辞書にあったものと同じ品詞で同じ長さなら使わない
    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext)  //末尾にシーク
            tmp_node = tmp_node->bnext;

        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            auto next_result = tmp_result_node->bnext;
            if (check_dict_match(tmp_result_node, dic_node)) {
                tmp_node->bnext = tmp_result_node;
                tmp_node = tmp_node->bnext;
                tmp_node->bnext = nullptr;
            } else {
                // std::cerr << "del unk_node:" <<
                // *(tmp_result_node->original_surface) << "_" <<
                // *(tmp_result_node->pos) << std::endl;
                delete tmp_result_node;
            }
            tmp_result_node = next_result;
        }
        tmp_node->bnext = nullptr;
        result_node = dic_node;
    } else {
        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            tmp_result_node->longer = true;
            // 素性の再生成
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_unigram_feature(tmp_result_node);
            tmp_result_node->wcost = f->calc_inner_product_with_weight();
            if (param->debug) {
                tmp_result_node->debug_info[
                    "unigram_feature"] = f->str();
            }
            if (tmp_result_node->feature) delete (tmp_result_node->feature);
            tmp_result_node->feature = f;

            tmp_result_node = tmp_result_node->bnext;
        }
    }

    return result_node;
}  //}}}

// start_str で始まる形態素を列挙する。
Node *Sentence::lookup_and_make_special_pseudo_nodes_lattice(  //{{{
    CharLattice &cl, const char *start_str, unsigned int char_num,
    unsigned int pos, unsigned int specified_length,
    std::string *specified_pos) {
    Node *result_node = NULL;
    // Node *kanji_result_node = NULL;

    // まず探す
    auto lattice_result = cl.da_search_from_position( dic->darts, char_num);  // こっちは何文字目かが必要

    // 以下は何バイト目かが必要
    // look up a dictionary with common prefix search
    Node *dic_node = dic->lookup_lattice(lattice_result, start_str + pos, specified_length, specified_pos);

    // オノマトペ処理
    Node *r_onomatope_node = dic->recognize_onomatopoeia(start_str + pos);
    if (r_onomatope_node != NULL) {
        if (dic_node) {  // 辞書の末尾につける
            Node *tmp_node = dic_node;
            while (tmp_node->bnext) tmp_node = tmp_node->bnext;
            tmp_node->bnext = r_onomatope_node;
        } else {
            dic_node = r_onomatope_node;
        }
    }

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
    // 同じ文字種が続く範囲を一語として入れてくれる
    if (specified_length || pos >= reached_pos_of_pseudo_nodes) {
        // 数詞処理
        result_node = dic->make_specified_pseudo_node_by_dic_check(
            start_str + pos, specified_length, specified_pos,
            &(param->unk_figure_pos), TYPE_FAMILY_FIGURE, nullptr);

        // alphabet
        if (!(param->passive_unknown) &&!result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_FAMILY_ALPH_PUNC);
        }
//      // 漢字
//      // カタカナ
//      // TODO:切れるところまでで未定義語を作る
        if (!(param->passive_unknown) && !result_node) {
            result_node = dic->make_specified_pseudo_node_by_dic_check(
                start_str + pos, specified_length, specified_pos,
                &(param->unk_pos), TYPE_KATAKANA, nullptr);
        }

        //
        if (!specified_length && result_node) // only prediction
            find_reached_pos_of_pseudo_nodes(pos, result_node);
    }

    // 辞書にあったものと同じ品詞で同じ長さなら使わない &
    // 辞書の長さより長いなら素性を追加
    // ここで一括チェックする方針
    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext)  //末尾にシーク
            tmp_node = tmp_node->bnext;
            
        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            auto next_result = tmp_result_node->bnext;
            if (check_dict_match(tmp_result_node, dic_node)) {
                tmp_node->bnext = tmp_result_node;
                tmp_node = tmp_node->bnext;
                tmp_node->bnext = nullptr;
            } else {
                // std::cerr << "del unk_node:" <<
                // *(tmp_result_node->original_surface) << "_" <<
                // *(tmp_result_node->pos) << std::endl;
                delete tmp_result_node;
            }
            tmp_result_node = next_result;
        }
        tmp_node->bnext = nullptr;
        result_node = dic_node;
    } else {  //辞書に無い場合
        Node *tmp_result_node = result_node;
        while (tmp_result_node) {
            tmp_result_node->longer = true;

            // 素性の再生成
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_unigram_feature(tmp_result_node);
            tmp_result_node->wcost = f->calc_inner_product_with_weight();
            if (param->debug) {
                tmp_result_node->debug_info[
                    "unigram_feature"] = f->str();
            }
            if (tmp_result_node->feature) delete (tmp_result_node->feature);
            tmp_result_node->feature = f;
            tmp_result_node = tmp_result_node->bnext;
        }
    }

    //    Node *tmp_result_node = result_node;
    //    while (tmp_result_node) {
    //        std::cerr << "lookup:" << *tmp_result_node->original_surface <<
    //        ":" << tmp_result_node->longer << std::endl;
    //        tmp_result_node = tmp_result_node->bnext;
    //    }

    return result_node;
}  //}}}

// TODO::本来はDic あたりに置く
bool Sentence::check_dict_match(Node *tmp_node, Node *dic_node) {  //{{{

    //辞書に一致する長さと品詞の形態素があればなければtrue, あればfalse
    if (!tmp_node) return false;
    if (tmp_node->sposid == dic->sposid2spos.get_id(
                                "数詞"))
        return true;  //数詞はチェックなしで使う

    Node *tmp_dic_node = dic_node;
    bool matched = false;
    unsigned longest_word = 0;
    // bool longer = false;

    // std::cerr << "check unk_node:" << *(tmp_node->base) << "_" <<
    // (tmp_node->posid) << std::endl;
    while (tmp_dic_node) {
        // std::cerr << "compare dict_node:" << *(tmp_dic_node->base) << "_" <<
        // (tmp_dic_node->posid) << " match:";

        // 辞書の中で最長の語長を保存
        if (longest_word < tmp_dic_node->length)
            longest_word = tmp_dic_node->length;

        if (tmp_node->length == tmp_dic_node->length &&  // length が一致
            tmp_node->posid == tmp_dic_node->posid) {    // pos が一致
            // std::cerr << "true" << endl;
            matched = true;
            //                break;
        }
        tmp_dic_node = tmp_dic_node->bnext;
    }

    if (tmp_node->length > longest_word) {
        tmp_node->longer = true;
        // 素性の再生成
        FeatureSet *f = new FeatureSet(ftmpl);
        f->extract_unigram_feature(tmp_node);
        tmp_node->wcost = f->calc_inner_product_with_weight();
        if (param->debug) {
            tmp_node->debug_info[
                "unigram_feature"] = f->str();
        }
        if (tmp_node->feature) delete (tmp_node->feature);
        tmp_node->feature = f;
    }

    return !matched;
}  //}}}

// 文の解析を行う
bool Sentence::lookup_and_analyze() {  //{{{
    lookup();
    // and
    analyze();
    return true;
}  //}}}

// 単語ラティスの生成
bool Sentence::lookup() {  //{{{
    unsigned int previous_pos = 0;
    unsigned int char_num = 0;

    // 文字Lattice の構築
    CharLattice cl;
    cl.parse(sentence_c_str);
    Node::reset_id_count();

    // TODO:ラティスを作った後，空いている pos 間を埋める未定義語を生成
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        if ((*end_node_list)[pos] == NULL) {  // pos にたどり着くノードが１つもない
            if (param->unknown_word_detection && pos > 0 && reached_pos <= pos)
                make_unk_pseudo_node_list_from_previous_position(sentence_c_str, previous_pos);
        } else {
            // make figure/alphabet nodes and look up a dictionary
            Node *r_node = lookup_and_make_special_pseudo_nodes_lattice(cl, sentence_c_str, char_num, pos, 0, NULL);

            set_begin_node_list(pos, r_node);
            find_reached_pos(pos, r_node);
            if (param->unknown_word_detection) {  // make unknown word candidates
                if (reached_pos <= pos) {        // ≒ pos で始まる単語が１つも無い場合
                    // cerr << ";; Cannot connect at position:" << pos << " ("
                    // << in_sentence << ")" << endl;
                    r_node = make_unk_pseudo_node_list_from_some_positions(sentence_c_str, pos, previous_pos);
                    
                    if(param->passive_unknown){
                        Node *result_node = NULL;
                        Node *tmp_node = r_node;
                        while (tmp_node->bnext)  //末尾にシーク
                            tmp_node = tmp_node->bnext;
                        
                        // カタカナなどの未定義語処理(仮
                        // alphabet
                        result_node = dic->make_specified_pseudo_node_by_dic_check(
                                sentence_c_str + pos, 
                                0, 
                                nullptr,
                                &(param->unk_pos), 
                                TYPE_FAMILY_ALPH_PUNC,
                                nullptr);
                            
                        // カタカナ
                        if (!result_node) {
                            result_node = dic->make_specified_pseudo_node_by_dic_check( sentence_c_str+pos, 0, nullptr, &(param->unk_pos), TYPE_KATAKANA, nullptr);
                        }
                        if (result_node){ 
                            find_reached_pos_of_pseudo_nodes(pos, result_node);
                            tmp_node->bnext = result_node;
                        }
                    }


                } else if (r_node && pos >= reached_pos_of_pseudo_nodes) {
                    for (unsigned int i = 2; i <= param->unk_max_length; i++) {
                        r_node = make_unk_pseudo_node_list_by_dic_check(sentence_c_str, pos, r_node, i);
                    }
                    
                    set_begin_node_list(pos, r_node);
                }
            }
            set_end_node_list(pos, r_node);
            if (param->use_so) set_bigram_info(pos, r_node);
        }
        previous_pos = pos;
        char_num++;
    }
    return true;
}  //}}}

// パスの選択
bool Sentence::analyze() {  //{{{
    // Viterbi
    // if(param->nbest){
    if (param->beam) {
        for (unsigned int pos = 0; pos < length;
             pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
            beam_at_position(pos, (*begin_node_list)[pos]);
        }
        find_best_beam();
    } else if (param->use_so) {  // second order viterbi
        for (unsigned int pos = 0; pos < length;
             pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
            so_viterbi_at_position(pos, (*begin_node_list)[pos]);
        }
        find_best_path_so();
    } else {  //普通のnbest
        for (unsigned int pos = 0; pos < length;
             pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
            viterbi_at_position_nbest(pos, (*begin_node_list)[pos]);
        }
        find_N_best_path();
    }
    return true;
}  //}}}

// 文のトピックを計算する
TopicVector Sentence::get_topic() {  //{{{
    TopicVector topic(TOPIC_NUM, 0.0);
    mark_nbest();

    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];

        while (node) {
            // n-best解に使われているもののみ
            std::set<int> used_length;
            if (node->used_in_nbest) {
                if (node->topic_available()) {
                    TopicVector node_topic = node->get_topic();

                    // count topic
                    if (used_length.count(node->char_num) ==
                        0) {  // 同じ開始位置で同じ長さの場合最初の物だけを使う
                        for (unsigned int i = 0; i < node_topic.size(); i++) {
                            topic[i] += node_topic[i];
                        }
                        used_length.insert(node->char_num);
                    }
                }
            }
            node = node->bnext;
        }
    }
    // 正規化
    double sum = 0.0;
    for (double x : topic) sum += x * x;
    if (sum != 0.0) {  //完全に0なら, 0ベクトルを返す
        for (double &x : topic) x /= sqrt(sum);
    }

    return topic;
}  //}}}

void Sentence::print_lattice() {  //{{{
    unsigned int char_num = 0;
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
        while (node) {
            for (unsigned int i = 0; i < char_num; i++) cerr << "  ";
            cerr << *(node->string_for_print);
            if (node->string_for_print != node->string)
                cerr << "(" << *(node->string) << ")";
            cerr << "_" << *(node->pos) << endl;
            node = node->bnext;
        }
        char_num++;
    }
}  //}}}

// 互換性のための旧フォーマットの出力
void Sentence::print_juman_lattice() {  //{{{

    mark_nbest();

    unsigned int char_num = 0;
    int id = 1;
    // 2 0 0 1 部屋 へや 部屋 名詞 6 普通名詞 1 * 0 * 0 “代表表記:部屋/へや
    // カテゴリ:場所-施設 …"
    // 15 2 2 2 に に に 助詞 9 格助詞 1 * 0 * 0 NIL
    // ID IDs_connecting_from index_begin index_end ...

    std::vector<std::vector<int>> num2id(length + 1);  //多めに保持する
    // cout << "len:" << length << endl;
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];

        while (node) {
            size_t word_length = node->char_num;
            // cout << "pos:" << pos << " wordlen:" << word_length << endl;
            // ID の表示
            if (node->used_in_nbest) {  // n-best解に使われているもののみ
                cout << id << " ";
                // 接続先
                num2id[char_num + word_length].push_back(id++);
                if (num2id[char_num].size() == 0) {  // 無かったら 0 を出す
                    cout << "0";
                } else {
                    std::string sep =
                        "";
                    for (auto to_id : num2id[char_num]) {
                        cout << sep << to_id;
                        sep =
                            ";";
                    }
                }
                cout << " ";
                // 文字index の表示
                cout << char_num << " " << char_num + word_length - 1 << " ";

                // 表層 よみ 原形
                if (*node->reading ==
                    "*") {  //読み不明であれば表層を使う
                    cout << *node->original_surface << " "
                         << *node->original_surface << " "
                         << *node->original_surface << " ";
                } else {
                    cout << *node->original_surface << " " << *node->reading
                         << " " << *node->base << " ";
                }
                if (*node->spos == UNK_POS) {
                    // 品詞 品詞id
                    cout << "未定義語"
                         << " " << Dic::pos_map.at(
                                       "未定義語") << " ";
                    // 細分類 細分類id
                    cout << "その他 " << Dic::spos_map.at(
                                             "その他") << " ";
                } else {
                    // 品詞 品詞id
                    cout << *node->pos << " " << Dic::pos_map.at(*node->pos)
                         << " ";
                    // 細分類 細分類id
                    cout << *node->spos << " " << Dic::spos_map.at(*node->spos)
                         << " ";
                }
                // 活用型 活用型id
                cout << *node->form_type << " "
                     << Dic::katuyou_type_map.at(*node->form_type) << " ";
                // 活用系 活用系id
                auto type_and_form = (*node->form_type +
                                      ":" + *node->form);
                if (Dic::katuyou_form_map.count(type_and_form) == 0)  //無い場合
                    cout << *node->form << " " << 0 << " ";
                else
                    cout << *node->form << " "
                         << Dic::katuyou_form_map.at(type_and_form) << " ";

                // 意味情報を再構築して表示
                if (*node->representation !=
                        "*" ||
                    *node->semantic_feature !=
                        "NIL" ||
                    *node->spos == UNK_POS) {
                    std::string delim =
                        "";
                    cout << '"';
                    if (*node->representation !=
                        "*") {
                        cout << "代表表記:"
                             << *node->representation;  //*ならスキップ
                        delim =
                            " ";
                    }
                    if (*node->semantic_feature !=
                        "NIL") {
                        cout << delim << *node->semantic_feature;  // NILならNIL
                        delim =
                            " ";
                    }
                    if (*node->spos == UNK_POS) {
                        cout << delim << "品詞推定:" << *node->pos << ":"
                             << *node->spos;
                        delim =
                            " ";
                    }
                    cout << '"' << endl;
                } else {
                    cout << "NIL" << endl;
                }
            }
            node = node->bnext;
        }
        char_num++;
    }
    cout << "EOS" << endl;
}  //}}}

void Sentence::print_unified_lattice() {  //{{{
    mark_nbest();

    unsigned int char_num = 0;
    // - 2 0 0 1 部屋 部屋/へや へや 部屋 名詞 6 普通名詞 1 * 0 * 0
    // "カテゴリ:場所-施設..."
    // - 15 2 2 2 に * に に 助詞 9 格助詞 1 * 0 * 0 NIL
    // wordmark ID fromIDs char_index_begin char_index_end surface rep_form
    // reading pos posid spos sposid form_type typeid form formid imis

    std::vector<std::vector<int>> num2id(length + 1);  //多めに保持する
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
        const std::string wordmark{
            "-"};
        const std::string delim{
            "\t"};

        while (node) {
            size_t word_length = node->char_num;
            // ID の表示
            if (node->used_in_nbest) {  // n-best解に使われているもののみ
                U8string ustr(*node->original_surface);
                cout << wordmark << delim << node->id << delim;
                num2id[char_num + word_length].push_back(
                    node->id);                       // 現在，接続先はn-best
                                                     // と関係なく繋がるもの全てを使用
                if (num2id[char_num].size() == 0) {  // 無かったら 0 を出す
                    cout << "0";
                } else {
                    std::string sep =
                        "";
                    for (auto to_id : num2id[char_num]) {
                        cout << sep << to_id;
                        sep =
                            ";";
                    }
                }
                cout << delim;
                // 文字index の表示
                cout << char_num << delim << char_num + word_length - 1
                     << delim;

                // 表層 代表表記 よみ 原形
                if (*node->reading ==
                    "*") {                                    //読み不明であれば表層を使う
                    cout << *node->original_surface << delim  // 表層
                         // 擬似代表表記を表示する
                         << *node->original_surface << "/"
                         << *node->original_surface << delim
                         << *node->original_surface << delim   // 読み
                         << *node->original_surface << delim;  // 原形
                } else {
                    if (*node->representation ==
                        "*") {  // Wikipedia 数詞など
                        cout << *node->original_surface << delim
                             << *node->original_surface << "/" << *node->reading
                             << delim << *node->reading << delim << *node->base
                             << delim;
                    } else {
                        cout << *node->original_surface << delim
                             << *node->representation << delim << *node->reading
                             << delim << *node->base << delim;
                    }
                }
                // 品詞 品詞id 細分類 細分類id
                if (*node->spos == UNK_POS) {
                    cout << "未定義語" << delim << Dic::pos_map.at(
                                                       "未定義語")
                         << delim;
                    if (ustr.is_katakana()) {
                        cout << "カタカナ" << delim
                             << Dic::spos_map.at(
                                    "カタカナ") << delim;
                    } else if (ustr.is_alphabet()) {
                        cout << "アルファベット" << delim
                             << Dic::spos_map.at(
                                    "アルファベット") << delim;
                    } else {
                        cout << "その他" << delim << Dic::spos_map.at(
                                                         "その他")
                             << delim;
                    }
                } else {
                    cout << *node->pos << delim << Dic::pos_map.at(*node->pos)
                         << delim;
                    cout << *node->spos << delim
                         << Dic::spos_map.at(*node->spos) << delim;
                }
                // 活用型 活用型id
                if (Dic::katuyou_type_map.count(*node->form_type) == 0) {
                    cout << "*" << delim << "0" << delim;
                } else {
                    cout << *node->form_type << delim
                         << Dic::katuyou_type_map.at(*node->form_type) << delim;
                }

                // 活用系 活用系id
                auto type_and_form = (*node->form_type +
                                      ":" + *node->form);
                if (Dic::katuyou_form_map.count(type_and_form) ==
                    0) {  //無い場合
                    if (*node->form ==
                        "<UNK>")
                        cout << "*" << delim << "0" << delim;
                    else
                        cout << *node->form << delim << "0" << delim;
                } else {
                    if (*node->form ==
                        "<UNK>") {
                        cout << "*" << delim << "0" << delim;
                    } else {
                        cout << *node->form << delim
                             << Dic::katuyou_form_map.at(type_and_form)
                             << delim;
                    }
                }

                // 意味情報を再構築して表示
                if (*node->semantic_feature !=
                        "NIL" ||
                    *node->spos == UNK_POS || ustr.is_katakana() ||
                    ustr.is_kanji() || ustr.is_eisuu() || ustr.is_kigou()) {
                    const std::string sep =
                        "|";
                    bool use_sep = false;

                    if (*node->semantic_feature !=
                        "NIL") {
                        // 一度空白で分割し,表示する
                        size_t current = 0, found;
                        while ((found = (*node->semantic_feature)
                                            .find_first_of(' ', current)) !=
                               std::string::npos) {
                            cout << (use_sep ? sep : "")
                                 << std::string(
                                        (*node->semantic_feature), current,
                                        found - current);  // NILでなければ
                            current = found + 1;
                            use_sep = true;
                        }
                        if (current == 0) {
                            cout << (use_sep ? sep : "")
                                 << *node->semantic_feature;
                            use_sep = true;
                        }
                    }
                    if (*node->spos == UNK_POS) {
                        cout << (use_sep ? sep : "")
                             << "品詞推定:" << *node->pos << ":" << *node->spos;
                        use_sep = true;
                    }
                    // TODO:これ以降の処理はあとでくくり出してNode 生成時に行う
                    bool kieisuuka = false;
                    if (ustr.is_katakana()) {
                        cout << (use_sep ? sep : "") << "カタカナ";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (ustr.is_kanji()) {
                        cout << (use_sep ? sep : "") << "漢字";
                        use_sep = true;
                    }
                    if (ustr.is_eisuu()) {
                        cout << (use_sep ? sep : "") << "英数字";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (ustr.is_kigou()) {
                        cout << (use_sep ? sep : "") << "記号";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (kieisuuka) {
                        cout << (use_sep ? sep : "") << "記英数カ";
                    }
                    cout << endl;
                } else {
                    cout << "NIL" << endl;
                }

                if (param->debug) {
                    // デバッグ出力
                    cout << "!\twcost(score):" << node->wcost
                         << "\tcost(score):" << node->cost << endl;
                    cout << "!\t" << node->debug_info[
                                         "unigram_feature"]
                         << endl;
                    std::stringstream ss_debug;
                    for (auto to_id : num2id[char_num]) {
                        ss_debug.str(
                            "");
                        ss_debug << to_id << " -> " << node->id;
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;

                        // node のbeam に残っているhistoryをたどる
                        for (auto tok : node->bq.beam) {
                            auto hist = tok.node_history;
                            if (hist.size() > 3 &&
                                hist[hist.size() - 2]->id == to_id) {
                                ss_debug.str(
                                    "");
                                ss_debug << hist[hist.size() - 3]->id << " -> "
                                         << hist[hist.size() - 2]->id << " -> "
                                         << node->id;
                                cout << "!\t" << ss_debug.str() << ": "
                                     << node->debug_info[ss_debug.str() +
                                                         ":trigram_feature"]
                                     << endl;
                            }
                        }
                    }
                    // BOS, EOS との接続の表示.. (TODO: 簡潔に書き換え)
                    ss_debug.str(
                        "");
                    ss_debug << -2 << " -> " << node->id;  // BOS
                    if (node->debug_info.find(ss_debug.str()) !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "BOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                    }
                    if (node->debug_info.find(ss_debug.str() +
                                              ":bigram_feature") !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "BOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;
                    }
                    ss_debug.str(
                        "");
                    ss_debug << node->id << " -> " << -1;  // EOS
                    if (node->debug_info.find(ss_debug.str()) !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "EOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                    }
                    if (node->debug_info.find(ss_debug.str() +
                                              ":bigram_feature") !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "EOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;
                    }
                }
            }
            node = node->bnext;
        }
        char_num++;
    }
    cout << "EOS" << endl;
}  //}}}

void Sentence::print_unified_lattice_rbeam(unsigned int nbest) {  //{{{
    mark_nbest_rbeam(nbest);

    unsigned int char_num = 0;
    // - 2 0 0 1 部屋 部屋/へや へや 部屋 名詞 6 普通名詞 1 * 0 * 0
    // "カテゴリ:場所-施設..."
    // - 15 2 2 2 に * に に 助詞 9 格助詞 1 * 0 * 0 NIL
    // wordmark ID fromIDs char_index_begin char_index_end surface rep_form
    // reading pos posid spos sposid form_type typeid form formid imis

    std::vector<std::vector<int>> num2id(length + 1);  //多めに保持する
    for (unsigned int pos = 0; pos < length;
         pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
        const std::string wordmark{
            "-"};
        const std::string delim{
            "\t"};

        while (node) {
            size_t word_length = node->char_num;
            // ID の表示
            if (node->used_in_nbest) {  // n-best解に使われているもののみ
                U8string ustr(*node->original_surface);
                cout << wordmark << delim << node->id << delim;
                num2id[char_num + word_length].push_back(
                    node->id);                       // 現在，接続先はn-best
                                                     // と関係なく繋がるもの全てを使用
                if (num2id[char_num].size() == 0) {  // 無かったら 0 を出す
                    cout << "0";
                } else {
                    std::string sep =
                        "";
                    for (auto to_id : num2id[char_num]) {
                        cout << sep << to_id;
                        sep =
                            ";";
                    }
                }
                cout << delim;
                // 文字index の表示
                cout << char_num << delim << char_num + word_length - 1
                     << delim;

                // 表層 代表表記 よみ 原形
                if (*node->reading ==
                    "*") {                                    //読み不明であれば表層を使う
                    cout << *node->original_surface << delim  // 表層
                         // 擬似代表表記を表示する
                         << *node->original_surface << "/"
                         << *node->original_surface << delim
                         << *node->original_surface << delim   // 読み
                         << *node->original_surface << delim;  // 原形
                } else {
                    if (*node->representation ==
                        "*") {  // Wikipedia 数詞など
                        cout << *node->original_surface << delim
                             << *node->original_surface << "/" << *node->reading
                             << delim << *node->reading << delim << *node->base
                             << delim;
                    } else {
                        cout << *node->original_surface << delim
                             << *node->representation << delim << *node->reading
                             << delim << *node->base << delim;
                    }
                }
                // 品詞 品詞id 細分類 細分類id
                if (*node->spos == UNK_POS) {
                    cout << "未定義語" << delim << Dic::pos_map.at(
                                                       "未定義語")
                         << delim;
                    if (ustr.is_katakana()) {
                        cout << "カタカナ" << delim
                             << Dic::spos_map.at(
                                    "カタカナ") << delim;
                    } else if (ustr.is_alphabet()) {
                        cout << "アルファベット" << delim
                             << Dic::spos_map.at(
                                    "アルファベット") << delim;
                    } else {
                        cout << "その他" << delim << Dic::spos_map.at(
                                                         "その他")
                             << delim;
                    }
                } else {
                    cout << *node->pos << delim << Dic::pos_map.at(*node->pos)
                         << delim;
                    cout << *node->spos << delim
                         << Dic::spos_map.at(*node->spos) << delim;
                }
                // 活用型 活用型id
                if (Dic::katuyou_type_map.count(*node->form_type) == 0) {
                    cout << "*" << delim << "0" << delim;
                } else {
                    cout << *node->form_type << delim
                         << Dic::katuyou_type_map.at(*node->form_type) << delim;
                }

                // 活用系 活用系id
                auto type_and_form = (*node->form_type +
                                      ":" + *node->form);
                if (Dic::katuyou_form_map.count(type_and_form) ==
                    0) {  //無い場合
                    if (*node->form ==
                        "<UNK>")
                        cout << "*" << delim << "0" << delim;
                    else
                        cout << *node->form << delim << "0" << delim;
                } else {
                    if (*node->form ==
                        "<UNK>") {
                        cout << "*" << delim << "0" << delim;
                    } else {
                        cout << *node->form << delim
                             << Dic::katuyou_form_map.at(type_and_form)
                             << delim;
                    }
                }

                // 意味情報を再構築して表示
                if (*node->semantic_feature != "NIL" ||
                    *node->spos == UNK_POS || ustr.is_katakana() ||
                    ustr.is_kanji() || ustr.is_eisuu() || ustr.is_kigou()) {
                    const std::string sep = "|";
                    bool use_sep = false;

                    if (*node->semantic_feature != "NIL") {
                        // 一度空白で分割し,表示する
                        size_t current = 0, found;
                        while ((found = (*node->semantic_feature)
                                            .find_first_of(' ', current)) !=
                               std::string::npos) {
                            cout << (use_sep ? sep : "")
                                 << std::string(
                                        (*node->semantic_feature), current,
                                        found - current);  // NILでなければ
                            current = found + 1;
                            use_sep = true;
                        }
                        if (current == 0) {
                            cout << (use_sep ? sep : "")
                                 << *node->semantic_feature;
                            use_sep = true;
                        }else if(current < (*node->semantic_feature).size() ){
                            cout << (use_sep ? sep : "")
                                 << std::string( (*node->semantic_feature), current,  (*node->semantic_feature).size() - current);
                        }
                    }
                    if (*node->spos == UNK_POS) {
                        cout << (use_sep ? sep : "")
                             << "品詞推定:" << *node->pos << ":" << *node->spos;
                        use_sep = true;
                    }
                    // TODO:これ以降の処理はあとでくくり出してNode 生成時に行う
                    bool kieisuuka = false;
                    if (ustr.is_katakana()) {
                        cout << (use_sep ? sep : "") << "カタカナ";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (ustr.is_kanji()) {
                        cout << (use_sep ? sep : "") << "漢字";
                        use_sep = true;
                    }
                    if (ustr.is_eisuu()) {
                        cout << (use_sep ? sep : "") << "英数字";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (ustr.is_kigou()) {
                        cout << (use_sep ? sep : "") << "記号";
                        use_sep = true;
                        kieisuuka = true;
                    }
                    if (kieisuuka) {
                        cout << (use_sep ? sep : "") << "記英数カ";
                    }
                    cout << endl;
                } else {
                    cout << "NIL" << endl;
                }

                if (param->debug) {
                    // デバッグ出力
                    cout << "!\twcost(score):" << node->wcost
                         << "\tcost(score):" << node->cost << endl;
                    cout << "!\t" << node->debug_info[
                                         "unigram_feature"]
                         << endl;
                    std::stringstream ss_debug;
                    for (auto to_id : num2id[char_num]) {
                        ss_debug.str(
                            "");
                        ss_debug << to_id << " -> " << node->id;
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                        cout << "!\t" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;

                        // node のbeam に残っているhistoryをたどる
                        for (auto tok : node->bq.beam) {
                            auto hist = tok.node_history;
                            if (hist.size() > 3 &&
                                hist[hist.size() - 2]->id == to_id) {
                                ss_debug.str(
                                    "");
                                ss_debug << hist[hist.size() - 3]->id << " -> "
                                         << hist[hist.size() - 2]->id << " -> "
                                         << node->id;
                                cout << "!\t" << ss_debug.str() << ": "
                                     << node->debug_info[ss_debug.str() +
                                                         ":trigram_feature"]
                                     << endl;
                            }
                        }
                    }
                    // BOS, EOS との接続の表示.. (TODO: 簡潔に書き換え)
                    ss_debug.str(
                        "");
                    ss_debug << -2 << " -> " << node->id;  // BOS
                    if (node->debug_info.find(ss_debug.str()) !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "BOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                    }
                    if (node->debug_info.find(ss_debug.str() +
                                              ":bigram_feature") !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "BOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;
                    }
                    ss_debug.str(
                        "");
                    ss_debug << node->id << " -> " << -1;  // EOS
                    if (node->debug_info.find(ss_debug.str()) !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "EOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str()] << endl;
                    }
                    if (node->debug_info.find(ss_debug.str() +
                                              ":bigram_feature") !=
                        node->debug_info.end()) {
                        cout << "!\t"
                             << "EOS:" << ss_debug.str() << ": "
                             << node->debug_info[ss_debug.str() +
                                                 ":bigram_feature"] << endl;
                    }
                }
            }
            node = node->bnext;
        }
        char_num++;
    }
    cout << "EOS" << endl;
}  //}}}

// Node か Dic でやるべき処理(get ではなく、BOSノードの生成)
Node *Sentence::get_bos_node() {  //{{{
    Node *bos_node = new Node;
    bos_node->surface = BOS;  // const_cast<const char *>(BOS);
    bos_node->string = new std::string(bos_node->surface);
    bos_node->string_for_print = new std::string(bos_node->surface);
    bos_node->end_string = new std::string(bos_node->surface);
    bos_node->original_surface =
        new std::string(BOS_STRING);  // ここはdelte される
    bos_node->length = 0;
    // bos_node->isbest = 1;
    bos_node->stat  = MORPH_BOS_NODE;
    bos_node->posid = MORPH_DUMMY_POS; //TODO: これ未知語と同じ品詞扱いになっている..
    bos_node->pos  = &(BOS_STRING);
    bos_node->spos = &(BOS_STRING);
    bos_node->form = &(BOS_STRING);
    bos_node->form_type = &(BOS_STRING);
    bos_node->base = &(BOS_STRING);
    bos_node->representation = &(BOS_STRING);
    bos_node->id = 0;

    // 初期 beam の作成
    bos_node->bq.beam.resize(1);
    bos_node->bq.beam.front().score = 0;
    bos_node->bq.beam.front().context_score = 0;
    bos_node->bq.beam.front().init_feature(ftmpl);
    bos_node->bq.beam.front().node_history.push_back(bos_node);
    bos_node->bq.beam.front().context = (Sentence::initial_context);

    FeatureSet *f = new FeatureSet(ftmpl);
    f->extract_unigram_feature(bos_node);
    bos_node->wcost = f->calc_inner_product_with_weight();
    bos_node->feature = f;

    return bos_node;
}  //}}}

Node *Sentence::get_eos_node() {  //{{{
    Node *eos_node = new Node;
    eos_node->surface = EOS;  // const_cast<const char *>(EOS);
    eos_node->string = new std::string(eos_node->surface);
    eos_node->string_for_print = new std::string(eos_node->surface);
    eos_node->end_string = new std::string(eos_node->surface);
    eos_node->original_surface = new std::string(EOS_STRING);
    eos_node->length = 1;  // dummy
    // eos_node->isbest = 1;
    eos_node->stat = MORPH_EOS_NODE;
    eos_node->posid = MORPH_DUMMY_POS; //TODO: これも
    eos_node->pos = &(EOS_STRING);
    eos_node->spos = &(EOS_STRING);
    eos_node->form = &(EOS_STRING);
    eos_node->form_type = &(EOS_STRING);
    eos_node->base = &(EOS_STRING);
    eos_node->representation = &(EOS_STRING);

    if (!param->use_so)  // so では eos のid が必要になるため
        eos_node->id = -1;

    FeatureSet *f = new FeatureSet(ftmpl);
    f->extract_unigram_feature(eos_node);
    eos_node->wcost = f->calc_inner_product_with_weight();
    eos_node->feature = f;

    return eos_node;
}  //}}}

// make EOS node and get the best path
Node *Sentence::find_best_path() {                //{{{
    (*begin_node_list)[length] = get_eos_node();  // End Of Sentence
    viterbi_at_position(length, (*begin_node_list)[length]);
    return (*begin_node_list)[length];
}  //}}}

Node *Sentence::find_N_best_path() {              //{{{
    (*begin_node_list)[length] = get_eos_node();  // End Of Sentence
    viterbi_at_position_nbest(length, (*begin_node_list)[length]);
    return (*begin_node_list)[length];
}  //}}}

Node *Sentence::find_best_beam() {                //{{{
    (*begin_node_list)[length] = get_eos_node();  // End Of Sentence
    beam_at_position(length, (*begin_node_list)[length]);
    return (*begin_node_list)[length];
}  //}}}

void Sentence::set_begin_node_list(unsigned int pos, Node *new_node) {  //{{{
    (*begin_node_list)[pos] = new_node;
}  //}}}

// TODO:いずれ nbest=1 の場合と統合 (現在は training にのみ利用)
bool Sentence::viterbi_at_position(unsigned int pos, Node *r_node) {  //{{{
    while (r_node) {
        double best_score = -DBL_MAX;
        Node *best_score_l_node = NULL;
        FeatureSet *best_score_bigram_f = NULL;
        Node *l_node = (*end_node_list)[pos];
        while (l_node) {
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_bigram_feature(l_node, r_node);
            if (param->trigram &&
                l_node->prev != nullptr)  // l_node よりも前がある
                f->extract_trigram_feature(l_node->prev, l_node, r_node);

            double bigram_score = f->calc_inner_product_with_weight();
            double score = l_node->cost + bigram_score + r_node->wcost;

            if (score > best_score) {
                best_score_l_node = l_node;
                if (best_score_bigram_f) delete best_score_bigram_f;
                best_score_bigram_f = f;
                best_score = score;
            } else {
                delete f;
            }
            l_node = l_node->enext;
        }

        if (best_score_l_node) {
            r_node->prev = best_score_l_node;
            r_node->next = NULL;
            r_node->cost = best_score;
            if (MODE_TRAIN) {  // feature collection
                r_node->feature->append_feature(best_score_l_node->feature);
                r_node->feature->append_feature(best_score_bigram_f);
            }
            delete best_score_bigram_f;
        } else {
            return false;
        }

        r_node = r_node->bnext;
    }

    return true;
}  //}}}

// Second order viterbi (使用していない)
bool Sentence::so_viterbi_at_position(unsigned int pos, Node *r_node) {  //{{{
    set_bigram_info(pos, r_node);

    if (pos == 0) return false;

    while (r_node) {
        Node *l_node = (*end_node_list)[pos];
        // set_bigram_info(pos-l_node->length, l_node);//チェックのため
        while (l_node) {
            Node *l2_node =
                (*end_node_list)[pos - l_node->length];  // l1_node のlength
            // を1にした分を引く?

            double best_score = -DBL_MAX;
            Node *best_score_l2_node = nullptr;
            FeatureSet *best_score_trigram_f = nullptr;
            while (l2_node) {
                // std::cerr << *l2_node->original_surface << " -> " <<
                // *l_node->original_surface << " -> " <<
                // *r_node->original_surface << std::endl;
                FeatureSet *f = new FeatureSet(ftmpl);
                f->extract_trigram_feature(l2_node, l_node, r_node);
                double trigram_score = f->calc_inner_product_with_weight();
                double score = l_node->get_bigram_total_cost(l2_node) +
                               r_node->get_bigram_cost(l_node) + trigram_score +
                               r_node->wcost;
                // l2_node->costは、l2_nodeとl_nodeにおけるbest_score
                // (l_node->wcostを含む; l2_nodeとl_nodeのbigram_scoreも含む)
                if (score > best_score) {
                    best_score_l2_node = l2_node;
                    if (best_score_trigram_f) delete best_score_trigram_f;
                    best_score_trigram_f = f;
                    best_score = score;
                } else {
                    delete f;
                }
                l2_node = l2_node->enext;
            }

            if (best_score_l2_node) {
                // std::cerr << "best:" << *best_score_l2_node->original_surface
                // << " -> " << *l_node->original_surface << " -> " <<
                // *r_node->original_surface << std::endl;
                r_node->set_bigram_best_prev(
                    l_node,
                    best_score_l2_node);  // このl_nodeとr_nodeに対するbest_l2_node
                r_node->set_bigram_total_cost(
                    l_node,
                    best_score);   // このl_nodeとr_nodeに対するbest_score
                if (MODE_TRAIN) {  // feature collection:
                                   // このl_nodeとr_nodeに対するfeature
                    r_node->append_bigram_feature(
                        l_node, l_node->get_bigram_feature(best_score_l2_node));
                    r_node->append_bigram_feature(l_node, best_score_trigram_f);
                }
                delete best_score_trigram_f;
            } else {
                std::cerr << "so viterbi failed!" << std::endl;
                return false;
            }
            l_node = l_node->enext;
        }
        r_node = r_node->bnext;
    }

    return true;
}  //}}}

bool Sentence::viterbi_at_position_nbest(unsigned int pos,
                                         Node *r_node) {  //{{{
    std::stringstream ss_key, ss_value;
    unsigned int traceSize = 0;

    while (r_node) {
        std::priority_queue<NbestSearchToken> nodeHeap;
        Node *l_node = (*end_node_list)[pos];
        FeatureSet best_score_bigram_f(ftmpl);
        double best_score = -DBL_MAX;

        while (l_node) {
            // 濁音化の条件チェック
            if ((r_node->stat ==
                 MORPH_DEVOICE_NODE) &&  //今の形態素が濁音化している
                (!check_devoice_condition(
                     *l_node))) {  //前の形態素が濁音化の条件を満たさない
                NbestSearchToken newSearchToken(-DBL_MAX, 0, l_node);
                nodeHeap.push(newSearchToken);
                l_node = l_node->enext;
                continue;
            }

            FeatureSet f(ftmpl);
            f.extract_bigram_feature(l_node, r_node);
            double bigram_score = f.calc_inner_product_with_weight();
            if (param->debug) {  //{{{
                ss_key.str(
                    ""),
                    ss_value.str(
                        "");
                ss_key << l_node->id << " -> " << r_node->id;
                ss_value << bigram_score << " + " << l_node->cost << " = "
                         << l_node->cost + bigram_score;
                r_node->debug_info[ss_key.str().c_str()] =
                    std::string(ss_value.str().c_str());
                l_node->debug_info[ss_key.str().c_str()] =
                    std::string(ss_value.str().c_str());
                r_node->debug_info[ss_key.str() +
                                   ":bigram_feature"] = f.str();
                l_node->debug_info[ss_key.str() +
                                   ":bigram_feature"] =
                    f.str();  // EOS用
            }                 //}}}

            // get_best_bigram_score
            if (l_node->cost + bigram_score + r_node->wcost > best_score) {
                best_score = l_node->cost + bigram_score + r_node->wcost;
                best_score_bigram_f = std::move(f);
            }

            traceSize = l_node->traceList.size();

            if (traceSize == 0) {
                double score = l_node->cost + bigram_score + r_node->wcost;
                NbestSearchToken newSearchToken(score, 0, l_node);
                nodeHeap.push(newSearchToken);

            } else {
                double last_score = DBL_MAX;
                for (unsigned int i = 0; i < traceSize; ++i) {
                    double score = l_node->traceList.at(i).score +
                                   bigram_score + r_node->wcost;
                    if (i > param->N_redundant &&
                        (last_score > score || i > param->N_redundant * 5))
                        break;
                    nodeHeap.emplace(score, i, l_node);
                    last_score = score;
                }
            }
            l_node = l_node->enext;
        }

        unsigned int heapSize = nodeHeap.size();

        double last_score = DBL_MAX;
        for (unsigned int i = 0; i < heapSize; ++i) {
            double score = nodeHeap.top().score;
            if (i > param->N_redundant && last_score > score) break;
            r_node->traceList.push_back(nodeHeap.top());
            nodeHeap.pop();
            last_score = score;
        }

        if (r_node->traceList.size() > 0) {
            r_node->next = NULL;
            r_node->prev = r_node->traceList.front().prevNode;
            r_node->cost = r_node->traceList.front().score;

            if (MODE_TRAIN) {  // feature collection
                r_node->feature->append_feature(
                    r_node->traceList.front().prevNode->feature);
                r_node->feature->append_feature(&(best_score_bigram_f));
            }
        } else {
            return false;
        }

        r_node = r_node->bnext;
    }

    return true;
}  //}}}

// 各形態素ノードごとにbeam_width 個もっておき，beam_search
bool Sentence::beam_at_position(unsigned int pos, Node *r_node) {  //{{{
    std::stringstream ss_key, ss_value;
    std::unordered_set<std::string> duplicate_filter;

    // r_node の重複を除外 (細分類まで一致する形態素を除外する)
    while (r_node) {  // pos から始まる形態素
        std::string key = (*r_node->original_surface + *r_node->base +
                           "_" +
                           *r_node->pos +
                           "_" + *r_node->spos +
                           "_" +
                           *r_node->form_type +
                           "_" + *r_node->form);
        // std::cerr << "key:" << key << std::endl;
        if (duplicate_filter.find(key) !=
            duplicate_filter.end()) {  //重複がある
            r_node = r_node->bnext;
            continue;
        } else {
            duplicate_filter.insert(key);
        }

        r_node->bq.beam.clear();
        r_node->bq.setN(param->N);
        Node *l_node = (*end_node_list)[pos];

        // 訓練時に必要になる trigram素性 の best も context
        // に含める必要がある(もしくは探索語に再生成)
        FeatureSet best_score_bigram_f(
            ftmpl);  // 訓練時には 1-best が必要になる
        FeatureSet best_score_trigram_f(
            ftmpl);  // 訓練時には 1-best が必要になる
        // double best_score = -DBL_MAX;
        // std::cerr << "r:" << *(r_node->original_surface) << "_" <<
        // *(r_node->base) << "_" << *(r_node->pos) << std::endl;

        while (l_node) {  // pos で終わる形態素
            if (l_node->bq.beam.size() == 0) {
                l_node = l_node->enext;
                continue;
            }
            // 1.コンテクスト独立の処理
            // std::cerr << "l:" << *(l_node->original_surface) << "_" <<
            // *(l_node->base) << "_" << (*l_node->pos) << std::endl;

            FeatureSet bi_f(ftmpl);
            bi_f.extract_bigram_feature(l_node, r_node);
            double bigram_score =
                (1.0 - param->rweight) * bi_f.calc_inner_product_with_weight();

            // 濁音化の条件チェック
            if ((r_node->stat ==
                 MORPH_DEVOICE_NODE) &&  // 今の形態素が濁音化している
                (!check_devoice_condition(
                     *l_node))) {  // 前の形態素が濁音化の条件を満たさない//{{{

                // 解析できないことを防ぐため，l_node の TOP 1 だけ一応処理する
                TokenWithState tok(r_node, l_node->bq.beam.front());
                tok.score = l_node->bq.beam.front().score + bigram_score +
                            (1.0 - param->rweight) * r_node->wcost -
                            10000;  // invalid devoice penalty
                tok.context_score = l_node->bq.beam.front().score - 10000;

                tok.f->append_feature(r_node->feature);  // unigram
                tok.f->append_feature(&bi_f);
                if (param->trigram &&
                    l_node->bq.beam.front().node_history.size() > 1) {
                    FeatureSet tri_f(ftmpl);
                    tri_f.extract_trigram_feature(
                        l_node->bq.beam.front().node_history
                            [l_node->bq.beam.front().node_history.size() - 2],
                        l_node, r_node);
                    tok.context_score += tri_f.calc_inner_product_with_weight();
                    tok.f->append_feature(&tri_f);
                }

                if (param->rnnlm) {
                    RNNLM::context new_c;
                    double rnn_score =
                        (param->rweight) *
                        rnnlm->test_word(l_node->bq.beam.front().context.get(),
                                         &new_c, *(r_node->base));
                    tok.context_score += (1.0 - param->rweight) * rnn_score;
                    tok.context =
                        std::make_shared<RNNLM::context>(std::move(new_c));
                }

                r_node->bq.push(std::move(tok));
                l_node = l_node->enext;
                continue;
            }  //}}}

            if (param->debug) {  //{{{
                ss_key.str(
                    ""),
                    ss_value.str(
                        "");
                ss_key << l_node->id << " -> " << r_node->id;
                ss_value << bigram_score << " + " << l_node->cost << " = "
                         << l_node->cost + bigram_score;
                r_node->debug_info[ss_key.str().c_str()] =
                    std::string(ss_value.str().c_str());
                l_node->debug_info[ss_key.str().c_str()] =
                    std::string(ss_value.str().c_str());
                r_node->debug_info[ss_key.str() +
                                   ":bigram_feature"] =
                    bi_f.str();
                l_node->debug_info[ss_key.str() +
                                   ":bigram_feature"] =
                    bi_f.str();  // EOS用
            }                    //}}}

            // std::cerr << *l_node->string << "  beam size:" <<
            // l_node->bq.beam.size() << std::endl;

            // 2.コンテクスト依存の処理
            for (auto &l_token_with_state :
                 l_node->bq.beam) {  // l_node で終わる形態素 top N
                // コンテクスト素性( RNNLM, tri-gram )
                RNNLM::context new_c;
                FeatureSet tri_f(ftmpl);
                // std::cout << "lw:" << *l_node->original_surface << ":" <<
                // *l_node->pos << " rw:" << *r_node->original_surface << ":" <<
                // *r_node->pos << std::endl;
                // std::cout << "last_word:" <<
                // (int)l_token_with_state.context->last_word << std::endl;
                // std::cout << "history_size:" <<
                // l_token_with_state.context->history.size() << std::endl;

                double trigram_score = 0.0;
                double rnn_score = 0.0;
                double srilm_score = 0.0;
                double context_score = l_token_with_state.context_score;
                double score = l_token_with_state.score + bigram_score +
                               (1.0 - param->rweight) * r_node->wcost;

                if (param->rnnlm) {
                    rnn_score =
                        (param->rweight) *
                        rnnlm->test_word(l_token_with_state.context.get(), &new_c, (*r_node->spos == UNK_POS|| *r_node->spos == "数詞" )?*(r_node->original_surface):*(r_node->base));
                    context_score += rnn_score;

                    if (param->debug)
                        std::cout << "lw:" << *l_node->original_surface << ":"
                                  << *l_node->pos
                                  << " rw:" << *r_node->original_surface << ":"
                                  << *r_node->pos << " => " << rnn_score
                                  << std::endl;
                } else if (param->srilm) {
                    unsigned int hist_size =
                        l_token_with_state.node_history.size();
                    unsigned int last_word = vocab->getIndex(
                        l_token_with_state.node_history.back()->base->c_str(), vocab->getIndex(Vocab_Unknown)); //<BOS>とかも変換する必要がある
                    //std::cerr << "hist_size: " << hist_size << std::endl;
                    unsigned int last_but_one_word = (hist_size > 3)? vocab->getIndex( 
                        l_token_with_state.node_history[hist_size - 3] ->base->c_str()): vocab->getIndex("<s>"); 
                    auto cur_base = (*r_node->spos == UNK_POS)?*(r_node->original_surface):*(r_node->base);
                    unsigned int cur_word =
                        vocab->getIndex(cur_base.c_str(), vocab->getIndex(Vocab_Unknown));
                    unsigned int context_word[] = {
                        last_word,
                        last_but_one_word,
                        Vocab_None
                    };  // l_token_with_state.context
                                             // からvocab.getIndex(word)
                                             // で取り出す

                    //std::cout << "cur_word(" << *r_node->base <<  ") = "  << cur_word << ", last_word = " << last_word << ", last_but_one_word = " << last_but_one_word << std::endl;
                        
                    // LogP Ngram::wordProb_Lpenalty(VocabIndex word, double Lpenalty, double cpenalty, int length,  const VocabIndex *context) {
                    //srilm_score = (param->rweight) * srilm->wordProb(cur_word, context_word);

                    //context_word の扱い, １つ前の単語，２つ前の単語, ... , n 前の単語
                    if (cur_word == vocab->unkIndex() || (context_word[0] == vocab->unkIndex()) || (context_word[1] == vocab->unkIndex())) 
                        srilm_score = (param->rweight) * ( -5.0 -(param->lweight * cur_base.length()/3.0)); //線形ペナルティ
                    else{
                        //std::cerr << "wordProb(" << cur_word << "," << context_word[0] << "," << context_word[1] <<  ")" << std::endl;
                        //srilm->setorder(2);
                        //std::cerr << "set order" << std::endl;
                        srilm_score = (param->rweight) * srilm->wordProb(cur_word, context_word);
                    }
                    context_score += srilm_score;
                }

                size_t history_size = l_token_with_state.node_history.size();
                if (param->trigram &&
                    history_size > 1) {  // 2つ前のノードがあれば.
                    tri_f.extract_trigram_feature(
                        l_token_with_state.node_history[history_size - 2],
                        l_node, r_node);
                    trigram_score = (1.0 - param->rweight) *
                                    tri_f.calc_inner_product_with_weight();
                    context_score += trigram_score;
                    if (param->debug) {  //{{{
                        ss_key.str(
                            ""),
                            ss_value.str(
                                "");
                        ss_key
                            << l_token_with_state.node_history[history_size - 2]
                                   ->id << " -> " << l_node->id << " -> "
                            << r_node->id;
                        ss_value << "cost:" << l_node->cost
                                 << " + bi:" << bigram_score
                                 << " + tri:" << trigram_score
                                 << " + rnn:" << rnn_score << " = "
                                 << context_score + score;
                        r_node->debug_info[ss_key.str().c_str()] =
                            ss_value.str();
                        l_node->debug_info[ss_key.str().c_str()] =
                            ss_value.str();
                        r_node->debug_info[ss_key.str() +
                                           ":trigram_feature"] =
                            tri_f.str();
                        l_node->debug_info[ss_key.str() +
                                           ":trigram_feature"] =
                            tri_f.str();  // EOS用
                    }                     //}}}
                }

                TokenWithState tok(r_node, l_token_with_state);
                tok.score = score;
                tok.context_score = context_score;

                // save the feature
                // tok.f->append_feature(r_node->feature); //unigram
                // tok.f->append_feature(&bi_f);
                // if(param->trigram)
                //    tok.f->append_feature(&tri_f);

                if (param->rnnlm)
                    tok.context =
                        std::make_shared<RNNLM::context>(std::move(new_c));
                r_node->bq.push(std::move(tok));
            }

            l_node = l_node->enext;
        }
        // std::cout << "bq.size = " << r_node->bq.beam.size() << std::endl;

        // 各r_node のnext prev ポインタ, featureを設定
        if (r_node->bq.beam.size() > 0) {
            r_node->next = nullptr;
            if (r_node->bq.beam.front().node_history.size() > 1) {
                r_node->prev =
                    r_node->bq.beam.front().node_history
                        [r_node->bq.beam.front().node_history.size() - 2];
            } else {
                r_node->prev = nullptr;
            }
            r_node->cost =
                r_node->bq.beam.front().score;  // context_score を入れる？
            // ほぼ表示用なので必要ない？
            if (MODE_TRAIN) {
                //(*r_node->feature) = (*r_node->bq.beam.front().f); //コピー
                ////使わないかも
            }
        }

        r_node = r_node->bnext;
    }

    // l_node->bq.beam.clear(); //メモリが気になるならしてもよい?
    return true;
}  //}}}

void Sentence::print_N_best_path() {  //{{{
                                      // 曖昧性のある形態素の出力
    std::string output_string_buffer;

    unsigned int N_required = param->N;
    unsigned int N_couter = 0;

    unsigned int traceSize = (*begin_node_list)[length]->traceList.size();
    if (traceSize > param->N_redundant) {
        traceSize = param->N_redundant;
    }

    for (unsigned int i = 0; i < traceSize; ++i) {
        Node *node = (*begin_node_list)[length];
        std::vector<Node *> result_morphs;

        bool find_bos_node = false;
        int traceRank = i;

        // Node* temp_node = NULL;
        double output_score = (*begin_node_list)[length]->traceList.at(i).score;

        while (node) {
            result_morphs.push_back(node);

            if (node->traceList.size() == 0) {
                break;
            }
            node = node->traceList.at(traceRank).prevNode;
            if (node->stat == MORPH_BOS_NODE) {
                find_bos_node = true;
                break;
            } else {
                traceRank = result_morphs.back()->traceList.at(traceRank).rank;
            }
        }

        if (!find_bos_node) cerr << ";; cannot analyze:" << sentence << endl;

        size_t printed_num = 0;
        for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
             it != result_morphs.rend(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;
                if (printed_num++) output_string_buffer.append(
                    " ");

                if (param->N > 1) {
                    output_string_buffer.append(*(*it)->string_for_print);
                    output_string_buffer.append(
                        "_");
                    output_string_buffer.append(*(*it)->pos);
                } else {
                    output_string_buffer.append(*(*it)->string_for_print);
                    output_string_buffer.append(
                        "_");
                    output_string_buffer.append(*(*it)->pos);
                    output_string_buffer.append(
                        ":");
                    output_string_buffer.append(*(*it)->spos);
                }
            }
        }

        std::map<std::string, int>::iterator find_output =
            nbest_duplicate_filter.find(output_string_buffer);
        if (find_output != nbest_duplicate_filter.end()) {
            // duplicate output
        } else {
            nbest_duplicate_filter.insert(
                std::make_pair(output_string_buffer, i));
            if (param->N > 1) cout << "# score:" << output_score << endl;
            cout << output_string_buffer;
            cout << endl;
            ++N_couter;
        }

        output_string_buffer.clear();
        if (N_couter >= N_required) break;
    }

    if (param->N > 1) cout << endl;
}  //}}}

void Sentence::print_N_best_with_rnn(RNNLM::CRnnLM &model) {  //{{{
    // 曖昧性のある形態素の出力
    std::string output_string_buffer;
    std::string rnnlm_string_buffer;

    unsigned int N_required = param->N;
    unsigned int N_couter = 0;

    unsigned int traceSize = (*begin_node_list)[length]->traceList.size();
    // if (traceSize > param->N_redundant) {
    //		traceSize = param->N_redundant;
    //}

    for (unsigned int i = 0; i < traceSize; ++i) {
        Node *node = (*begin_node_list)[length];
        std::vector<Node *> result_morphs;

        bool find_bos_node = false;
        int traceRank = i;

        // Node* temp_node = NULL;
        double output_score = (*begin_node_list)[length]->traceList.at(i).score;

        while (node) {
            result_morphs.push_back(node);

            if (node->traceList.size() == 0) {
                break;
            }
            node = node->traceList.at(traceRank).prevNode;
            if (node->stat == MORPH_BOS_NODE) {
                find_bos_node = true;
                break;
            } else {
                traceRank = result_morphs.back()->traceList.at(traceRank).rank;
            }
        }

        if (!find_bos_node) cerr << ";; cannot analyze:" << sentence << endl;

        size_t printed_num = 0;
        for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
             it != result_morphs.rend(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;
                if (printed_num++) {
                    output_string_buffer.append(
                        " ");
                    rnnlm_string_buffer.append(
                        " ");
                }
                output_string_buffer.append(*(*it)->string_for_print);
                rnnlm_string_buffer.append(*(*it)->base);
                output_string_buffer.append(
                    "_");
                output_string_buffer.append(*(*it)->pos);
            }
        }

        std::map<std::string, int>::iterator find_output =
            nbest_duplicate_filter.find(output_string_buffer);
        if (find_output != nbest_duplicate_filter.end()) {
            // duplicate output
        } else {
            nbest_duplicate_filter.insert(
                std::make_pair(output_string_buffer, i));
            double rnnlm_score = model.test_sent(rnnlm_string_buffer);
            cout << "# score:" << output_score << " rnnlm:" << rnnlm_score
                 << endl;
            cout << output_string_buffer;
            cout << endl;
            ++N_couter;
        }

        output_string_buffer.clear();
        rnnlm_string_buffer.clear();
        if (N_couter >= N_required) break;
    }
    cout << endl;
}  //}}}

// beam サーチ用の出力関数
void Sentence::print_beam() {  //{{{
    std::string output_string_buffer;
    std::unordered_set<std::string> nbest_duplicate_filter;
    auto &beam = (*begin_node_list)[length]->bq.beam;

    // std::sort(beam.begin(), beam.end(),[](auto &x, auto &y){return
    // x.score+x.context_score > y.score+y.context_score;});
    for (auto &token :
         (*begin_node_list)[length]->bq.beam) {  //最後は必ず EOS のみ
        std::vector<Node *> result_morphs = token.node_history;

        size_t printed_num = 0;
        for (auto it = result_morphs.begin(); it != result_morphs.end(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;
                if (printed_num++) output_string_buffer.append(
                    " ");
                output_string_buffer.append(*(*it)->string_for_print);
                output_string_buffer.append(
                    "_");
                output_string_buffer.append(*(*it)->pos);
                output_string_buffer.append(
                    ":");
                output_string_buffer.append(*(*it)->spos);
            }
        }

        // auto find_output = nbest_duplicate_filter.find(output_string_buffer);
        // if(find_output == nbest_duplicate_filter.end()){
        cout << "# score:" << token.score
             << " # context: " << token.context_score << endl;
        cout << output_string_buffer << endl;
        //}
        output_string_buffer.clear();
    }
}  //}}}

void Sentence::print_best_beam() {  //{{{
    std::string output_string_buffer;

    for (auto &token : (*begin_node_list)[length]->bq.beam) {  //最後は必ず EOS のみ
        std::vector<Node *> result_morphs = token.node_history;

        size_t printed_num = 0;
        for (auto it = result_morphs.begin(); it != result_morphs.end(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;
                if (printed_num++) output_string_buffer.append(
                    " ");
                output_string_buffer.append(*(*it)->string_for_print);
                output_string_buffer.append(
                    "_");
                output_string_buffer.append(*(*it)->pos);
                output_string_buffer.append(
                    ":");
                output_string_buffer.append(*(*it)->spos);
            }
        }

        // cout << "# score:" << token.score << endl;
        std::cout << output_string_buffer << std::endl;
        output_string_buffer.clear();

        return;
    }
}  //}}}

void Sentence::print_best_beam_juman() {  //{{{
    std::stringstream output_string_buffer;

    for (auto &token :
         (*begin_node_list)[length]->bq.beam) {  //最後は必ず EOS のみ
        std::vector<Node *> result_morphs = token.node_history;

        for (auto it = result_morphs.begin(); it != result_morphs.end(); it++) {
            auto & node = (*it);
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {

                if (*node->reading == "*"|| *node->spos == UNK_POS || *node->spos == "数詞") {  //読み不明であれば表層を使う
                    output_string_buffer << *node->original_surface << " "
                        << *node->original_surface << " "
                        << *node->original_surface << " ";
                } else {
                    output_string_buffer << *node->original_surface << " " << *node->reading
                         << " " << *node->base << " ";
                }
                if (*node->spos == UNK_POS) {
                    // 品詞 品詞id
                    output_string_buffer << "未定義語" << " " << Dic::pos_map.at( "未定義語") << " ";
                    // 細分類 細分類id
                    output_string_buffer << "その他 " << Dic::spos_map.at( "その他") << " ";
                } else {
                    // 品詞 品詞id
                    output_string_buffer << *node->pos << " " << Dic::pos_map.at(*node->pos) << " ";
                    // 細分類 細分類id
                    output_string_buffer << *node->spos << " " << Dic::spos_map.at(*node->spos) << " ";
                }
                // 活用型 活用型id
                if (Dic::katuyou_type_map.count(*node->form_type) == 0)  //無い場合
                    output_string_buffer << "*" << " " << 0 << " ";
                else
                    output_string_buffer << *node->form_type << " " << Dic::katuyou_type_map.at(*node->form_type) << " ";
                // 活用系 活用系id
                auto type_and_form = (*node->form_type + ":" + *node->form);
                if (Dic::katuyou_form_map.count(type_and_form) == 0)  //無い場合
                    output_string_buffer << "*" << " " << 0 << " ";
                else
                    output_string_buffer << *node->form << " "
                         << Dic::katuyou_form_map.at(type_and_form) << " ";

                // 意味情報を再構築して表示
                if (*node->representation != "*" ||
                    *node->semantic_feature != "NIL" ||
                    *node->spos == UNK_POS) {
                    std::string delim = "";
                    output_string_buffer << '"';
                    if (*node->representation != "*") {
                        output_string_buffer << "代表表記:" << *node->representation;  //*ならスキップ
                        delim = " ";
                    }
                    if (*node->semantic_feature != "NIL") {
                        output_string_buffer << delim << *node->semantic_feature; // NILならNIL
                        delim = " ";
                    }
                    if (*node->spos == UNK_POS) {
                        output_string_buffer << delim << "品詞推定:" << *node->pos << ":" << *node->spos;
                        delim = " ";
                    }
                    output_string_buffer << std::string("\"") << endl;
                } else {
                    output_string_buffer << std::string("NIL") << endl;
                }
            }
        }
            
        std::cout << output_string_buffer.str() << "EOS" << std::endl;

        return;
    }
}  //}}}

// ラティス出力する時のためにn-best に含まれる形態素をマーキングする
void Sentence::mark_nbest() {  //{{{
    unsigned int traceSize_original =
        (*begin_node_list)[length]->traceList.size();
    unsigned int traceSize = (*begin_node_list)[length]->traceList.size();
    if (traceSize > param->N_redundant) {
        traceSize = param->N_redundant;
    }

    // 近いスコアの場合をまとめるために，整数に丸める
    double last_score = DBL_MAX;
    long sample_num = 0;
    unsigned int i = 0;

    while (i < traceSize_original) {
        Node *node = (*begin_node_list)[length];
        std::vector<Node *> result_morphs;
        bool find_bos_node = false;
        int traceRank = i;

        // Node* temp_node = NULL;
        double output_score = (*begin_node_list)[length]->traceList.at(i).score;
        if (last_score > output_score)
            sample_num++;  // 同スコアの場合は数に数えず，N-bestに出力
        if (sample_num > param->N || i > param->N * 5) break;

        while (node) {
            result_morphs.push_back(node);

            if (node->traceList.size() == 0) {
                break;
            }
            node = node->traceList.at(traceRank).prevNode;
            if (node->stat == MORPH_BOS_NODE) {
                find_bos_node = true;
                break;
            } else {
                traceRank = result_morphs.back()->traceList.at(traceRank).rank;
            }
        }

        if (!find_bos_node) {
            cerr << ";; cannot analyze:" << sentence << endl;
        }

        // size_t printed_num = 0;
        unsigned long byte_pos = 0;
        // 後ろから追加しているので、元の順にするために逆向き
        for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
             it != result_morphs.rend(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest =
                    true;  // Nodeにnbestに入っているかをマークする

                if (output_ambiguous_word) {
                    auto tmp = (*begin_node_list)[byte_pos];
                    while (
                        tmp) {  //同じ長さ(同じ表層)で同じ表層のものをすべて出力する
                        if (tmp->length == (*it)->length &&
                            tmp->posid == (*it)->posid) {
                            tmp->used_in_nbest = true;
                        }
                        tmp = tmp->bnext;
                    }
                }
                byte_pos += (*it)->length;
            }
        }
        i++;
        last_score = output_score;
    }
}  //}}}

void Sentence::mark_nbest_rbeam(unsigned int nbest) {  //{{{
    auto &beam = (*begin_node_list)[length]->bq.beam;
    unsigned int traceSize = beam.size();
    if (traceSize > nbest){
        traceSize = nbest;
    }
        
    unsigned int i = 0;

    for (auto &token : (*begin_node_list)[length]->bq.beam) {  //最後は必ず EOS のみ
        std::vector<Node *> result_morphs = token.node_history;
        if(++i > traceSize)break;

        unsigned int byte_pos = 0;
        for (auto it = result_morphs.begin(); it != result_morphs.end(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE && (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;

                if (output_ambiguous_word) {
                    auto tmp = (*begin_node_list)[byte_pos];
                    // cout << *((*it)->representation) << " " << byte_pos << endl;
                    while (tmp) { //同じ長さ(同じ表層)で同じ品詞のものをすべて出力する
                        if (tmp->length == (*it)->length &&
                            tmp->posid == (*it)->posid &&
                            tmp->sposid == (*it)->sposid &&
                            tmp->baseid == (*it)->baseid &&
                            // 助詞と判定詞の異なりを含める
                            tmp->formid == (*it)->formid 
                            //tmp->formtypeid == (*it)->formtypeid
                            ) {
                            tmp->used_in_nbest = true;
                        }
                        tmp = tmp->bnext;
                    }
                }
                byte_pos += (*it)->length;
            }
        }
    }
}  //}}}

// update end_node_list
void Sentence::set_end_node_list(unsigned int pos, Node *r_node) {  //{{{
    while (r_node) {
        if (r_node->stat != MORPH_EOS_NODE) {
            unsigned int end_pos = pos + r_node->length;
            r_node->enext = (*end_node_list)[end_pos];
            (*end_node_list)[end_pos] = r_node;
        }
        r_node = r_node->bnext;
    }
}  //}}}

unsigned int Sentence::find_reached_pos(unsigned int pos, Node *node) {  //{{{
    while (node) {
        unsigned int end_pos = pos + node->length;
        if (end_pos > reached_pos) reached_pos = end_pos;
        node = node->bnext;
    }
    return reached_pos;
}  //}}}

// reached_pos_of_pseudo_nodes の計算、どこまでの範囲を未定義語として入れたか
unsigned int Sentence::find_reached_pos_of_pseudo_nodes(unsigned int pos, Node *node) {  //{{{
    while (node) {
        if (node->stat == MORPH_UNK_NODE) {
            unsigned int end_pos = pos + node->length;
            if (end_pos > reached_pos_of_pseudo_nodes)
                reached_pos_of_pseudo_nodes = end_pos;
        }
        node = node->bnext;
    }
    return reached_pos_of_pseudo_nodes;
}  //}}}

void Sentence::print_best_path() {  //{{{
    Node *node = (*begin_node_list)[length];
    std::vector<Node *> result_morphs;

    bool find_bos_node = false;
    while (node) {
        if (node->stat == MORPH_BOS_NODE) find_bos_node = true;
        result_morphs.push_back(node);
        node = node->prev;
    }

    if (!find_bos_node) cerr << ";; cannot analyze:" << sentence << endl;

    size_t printed_num = 0;
    for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
         it != result_morphs.rend(); it++) {
        if ((*it)->stat != MORPH_BOS_NODE && (*it)->stat != MORPH_EOS_NODE) {
            if (printed_num++) cout << " ";
            (*it)->print();
        }
    }
    cout << endl;
}  //}}}

void Sentence::print_best_path_with_rnn(RNNLM::CRnnLM &model) {  //{{{

    std::vector<std::pair<double, std::string>> nbest_pathes;

    std::string output_string_buffer;
    std::string rnnlm_string_buffer;

    unsigned int N_required = param->N;
    unsigned int N_couter = 0;

    // cout << param->N << endl;

    unsigned int traceSize = (*begin_node_list)[length]->traceList.size();

    // cout << traceSize << endl;

    for (unsigned int i = 0; i < traceSize; ++i) {
        Node *node = (*begin_node_list)[length];
        std::vector<Node *> result_morphs;

        bool find_bos_node = false;
        int traceRank = i;
        double output_score = (*begin_node_list)[length]->traceList.at(i).score;

        while (node) {
            result_morphs.push_back(node);

            if (node->traceList.size() == 0) {
                break;
            }
            node = node->traceList.at(traceRank).prevNode;
            if (node->stat == MORPH_BOS_NODE) {
                find_bos_node = true;
                break;
            } else {
                traceRank = result_morphs.back()->traceList.at(traceRank).rank;
            }
        }

        if (!find_bos_node) cerr << ";; cannot analyze:" << sentence << endl;

        size_t printed_num = 0;
        for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
             it != result_morphs.rend(); it++) {
            if ((*it)->stat != MORPH_BOS_NODE &&
                (*it)->stat != MORPH_EOS_NODE) {
                (*it)->used_in_nbest = true;
                if (printed_num++) {
                    output_string_buffer.append(
                        " ");
                    rnnlm_string_buffer.append(
                        " ");
                }
                output_string_buffer.append(*(*it)->string_for_print);
                output_string_buffer.append(
                    "_");
                output_string_buffer.append(*(*it)->pos);
                output_string_buffer.append(
                    ":");
                output_string_buffer.append(*(*it)->spos);
                rnnlm_string_buffer.append(*(*it)->base);
            }
        }

        std::map<std::string, int>::iterator find_output =
            nbest_duplicate_filter.find(output_string_buffer);
        if (find_output != nbest_duplicate_filter.end()) {
            // duplicate output
            // cout << "dup: " << output_string_buffer << endl;
        } else {
            nbest_duplicate_filter.insert(
                std::make_pair(output_string_buffer, i));
            double rnnlm_score = model.test_sent(rnnlm_string_buffer);

            nbest_pathes.push_back(std::make_pair(
                output_score + rnnlm_score, std::string(output_string_buffer)));
            // cout << "# score:" << output_score << " rnnlm:" << rnnlm_score <<
            // endl;
            // cout << rnnlm_string_buffer;
            // cout << endl;
            ++N_couter;
        }

        output_string_buffer.clear();
        rnnlm_string_buffer.clear();
        if (N_couter >= N_required) break;
    }

    // 1-best を出力
    // std::sort(nbest_pathes.begin(), nbest_pathes.end(),[](const auto &x,
    // const auto &y){ return x.first < y.first;});
    std::sort(nbest_pathes.begin(), nbest_pathes.end(),
              std::greater<std::pair<double, std::string>>());
    // cout << "sorted"<< endl;
    // cout << "size:" << nbest_pathes.size() << endl;

    // cout << nbest_pathes[0].first << endl;
    cout << nbest_pathes.front().second << endl;

    // cout << endl;
}  //}}}

// gold 用の辞書引きの亜種
bool Sentence::lookup_gold_data(std::string &word_pos_pair) {  //{{{
    if (reached_pos < length) {
        cerr << ";; ERROR! Cannot connect at position for gold: "
             << word_pos_pair << endl;
    }

    std::vector<std::string> line(7);
    split_string(word_pos_pair,
                 "_", line);
    Node::reset_id_count();
    std::vector<std::string> spec{
        line[1], line[2], line[3],
        line[4], line[5], line[6]};  // reading,base,pos, spos, formtype, form

    // TODO: 外国人人名 が苗字と名前がくっついていても引けるようにしたい．

    CharLattice cl;
    cl.parse(line[0]);

    // そのまま辞書を引く
    // Node *r_node = lookup_and_make_special_pseudo_nodes(line[0].c_str(),
    // strlen(line[0].c_str()), spec);
    Node *r_node = lookup_and_make_special_pseudo_nodes_lattice(
        cl, line[0].c_str(), strlen(line[0].c_str()), spec);

    if (!r_node &&
        (line[6] ==
             "命令形エ形" ||
         line[6] ==
             "命令形")) {  // JUMANの活用になく，３文コーパスでのみ出てくる活用の命令形エ形を連用形だと解釈する.
        std::vector<std::string> mod_spec{line[1],
                                          "", line[3],
                                          line[4], line[5],
                                          "基本連用形"};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore 命令形エ形 verb:" <<  line[0] << ":" << line[1] <<
        // ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" << line[5]
        // << ":" << line[6] << endl;
    }

    if (!r_node && line[3] ==
                       "名詞") {  // 動詞の名詞化を復元
        // コーパスでは名詞化した動詞は名詞として扱われる. (kkn
        // では動詞の連用形扱い)
        // 品詞変更タグがついていない場合の対策
        // 動詞の活用型不明のまま，基本連用形をチェック
        std::vector<std::string> mod_spec{line[1],
                                          "",
                                          "動詞",
                                          "*",
                                          "",
                                          "基本連用形"};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore nominalized verb:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }

    if (!r_node && line[3] ==
                       "名詞") {  // 形容詞語幹を名詞と表記している場合
        // コーパスでは"綺麗"などが名詞として使われることがあるが，辞書では"綺麗"は形容詞のみの場合
        std::vector<std::string> mod_spec{line[1],
                                          "",
                                          "形容詞",
                                          "",
                                          "ナ形容詞",
                                          "語幹"};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore nominalized verb:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }

    if (!r_node && line[3] ==
                       "名詞") {  // 名詞扱いの副詞
        // コーパスでは"事実"などが名詞として使われるが，辞書では"事実"は副詞のみ
        std::vector<std::string> mod_spec{line[1], line[2],
                                          "副詞",
                                          "",
                                          "",
                                          ""};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore nominalized verb:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }

    if (!r_node && line[3] ==
                       "副詞") {  // 副詞扱いの名詞
        std::vector<std::string> mod_spec{line[1], line[2],
                                          "名詞",
                                          "",
                                          "",
                                          ""};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
    }

    if (!r_node && line[3] ==
                       "副詞") {  // 副詞扱いの形容詞 名詞も
        // コーパスでは"真実"などが副詞として使われるが，辞書では"事実"は形容詞のみ
        std::vector<std::string> mod_spec{line[1], line[2],
                                          "形容詞",
                                          "",
                                          "",
                                          ""};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore nominalized verb:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }
    // 人名, 地名が辞書にない場合 // 未定義語として処理

    if (!r_node) {  // 活用型が誤っている場合があるので，活用型を無視する
        std::vector<std::string> mod_spec{line[1], line[2], line[3],
                                          line[4],
                                          "", line[6]};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore nominalized verb:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }

    if (!r_node) {  // 濁音化, 音便化している(?相:しょう,など)しているケース
        // は読みを無視して検索
        std::vector<std::string> mod_spec{
            "", line[2], line[3],
            line[4], line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; restore voiced reading:" <<  line[0] << ":" << line[1] <<
        // ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" << line[5]
        // << ":" << line[6] << endl;
    }

    // 名詞が固有名詞, サ変名詞化している場合, 細分類を無視して辞書引き
    // Wikipedia 辞書で読みが正しく付与されていない場合があるので，読みも無視
    if (!r_node && line[3] ==
                       "名詞") {
        std::vector<std::string> mod_spec{
            "", line[2], line[3],
            "", line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
        // cerr << "; modify proper noun to noun:" <<  line[0] << ":" << line[1]
        // << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":" <<
        // line[5] << ":" << line[6] << endl;
    }

    // 記号に読み方が書かれている場合は無視する
    if (!r_node && line[3] ==
                       "特殊") {
        std::vector<std::string> mod_spec{
            "", line[2], line[3],
            line[4], line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
    }

    // 長音が挿入されている場合, 読みと原形を無視する
    if (!r_node && (line[0].find(
                        "〜", 0) != std::string::npos ||
                    line[0].find(
                        "ー", 0) != std::string::npos)) {
        std::vector<std::string> mod_spec{
            "",
            "", line[3],
            line[4], line[5], line[6]};
        r_node = lookup_and_make_special_pseudo_nodes_lattice(
            cl, line[0].c_str(), strlen(line[0].c_str()), mod_spec);
    }

    if (!r_node) {  // 未定義語として処理
        // 細分類以下まで推定するなら，以下も書き換える
        r_node = dic->make_unk_pseudo_node_gold(
            line[0].c_str(), strlen(line[0].c_str()), line[3]);
        cerr << ";; lookup failed in gold data:" << line[0] << ":" << line[1]
             << ":" << line[2] << ":" << line[3] << ":" << line[4] << ":"
             << line[5] << ":" << line[6] << endl;
    }

    // auto tmp_node = r_node;
    // while(tmp_node){// 全部表示
    //    std::cerr << *tmp_node->original_surface << " " << *tmp_node->base <<
    //    " " <<  *tmp_node->pos << " " << *tmp_node->spos << " " <<
    //    *tmp_node->semantic_feature << std::endl;
    //    tmp_node=tmp_node->bnext;
    //}

    (*begin_node_list)[length] = r_node;
    find_reached_pos(length, r_node);

    if (param->beam) {
        beam_at_position(length, r_node);
    } else if (param->use_so) {
        so_viterbi_at_position(length, r_node);
    } else {
        viterbi_at_position(length, r_node);
    }
    set_end_node_list(length, r_node);

    add_one_word(line[0]);
    return true;
}  //}}}

// gold とどの程度離れた解析をしたか、0-1の範囲で返す
double Sentence::eval(Sentence &gold) {  //{{{
    if (length != gold.length) {
        cerr << ";; cannot calc loss " << sentence << endl;
        return 1.0;
    }

    Node *node = (*begin_node_list)[length];
    std::vector<Node *> result_morphs;

    bool find_bos_node = false;
    while (node) {  // 1-best
        if (node->stat == MORPH_BOS_NODE) find_bos_node = true;
        result_morphs.push_back(node);
        node = node->prev;
    }

    if (!find_bos_node) {
        cerr << ";; cannot analyze:" << sentence << endl;
        for (auto &m : result_morphs) {
            cerr << *(m->original_surface) << "  ";
        }
        cerr << endl;
        cerr << gold.sentence << endl;
    }

    auto itr = result_morphs.rbegin();
    auto itr_gold = gold.gold_morphs.rbegin();

    size_t byte = 0;
    size_t byte_gold = 0;
    double score = 0.0;
    size_t morph_count = 0;
    // while (itr_gold != gold.gold_morphs.rend() && itr !=
    // result_morphs.rend()){
    while (itr_gold != gold.gold_morphs.rend()) {
        if (byte_gold > byte) {
            byte += (*itr)->length;
            itr++;
        } else if (byte > byte_gold) {
            byte_gold += (itr_gold)->length;
            itr_gold++;
            morph_count++;
        } else {  // byte == byte_gold
            if ((*itr)->length ==
                (itr_gold)->length) {  // 同じ場所に同じ長さがあれば0.5点
                score += 0.5;
                if ((*itr)->posid ==
                    (itr_gold)->posid) {  //同じ品詞 なら追加で0.25点
                    score += 0.25;
                    if ((*itr)->sposid ==
                        (itr_gold)->sposid)  //さらに同じ細分類 なら追加で0.25点
                        score += 0.25;
                }
            }
            // cerr << *(*itr)->base << " "<< *(itr_gold)->base << endl;
            byte += (*itr)->length;
            byte_gold += (itr_gold)->length;
            itr++;
            itr_gold++;
            morph_count++;
        }
    }
    // TODO:BOS EOS を除く
    return 1.0 - (score / morph_count);
};  //}}}

std::vector<std::string> Sentence::get_gold_topic_features(
    TopicVector *tov) {  //{{{
    if (tov) {
        std::stringstream topic_feature;
        FeatureSet f(ftmpl);
        TopicVector *tmp_topic = FeatureSet::topic;  // 一時保存
        FeatureSet::topic = tov;
        for (Node node : gold_morphs) {
            if (!node.is_dummy()) {
                f.extract_topic_feature(&node);
                // std::cerr << *(node.base) << "(" << node.topic_available() <<
                // ")"<< std::endl;
            }
        }
        FeatureSet::topic = tmp_topic;

        return std::move(f.fset);
    } else {
        return std::vector<std::string>();
    }
}  //}}}

// パーセプトロン専用の関数，いずれ撲滅
// TODO: 廃止
void Sentence::minus_feature_from_weight(
    Umap &in_feature_weight, size_t factor) {  //{{{
    Node *node = (*begin_node_list)[length];                                      // EOS
    node->feature->minus_feature_from_weight(in_feature_weight, factor);
}  //}}}

// TODO: 廃止
void Sentence::minus_feature_from_weight(
    Umap &in_feature_weight) {  //{{{
    minus_feature_from_weight(in_feature_weight, 1);
}  //}}}
}

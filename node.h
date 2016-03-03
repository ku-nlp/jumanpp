#pragma once
#ifndef NODE_H
#define NODE_H

#include "common.h"
#include "pos.h"
extern "C"{
#include "cdb_juman.h"
}
#include "scw.h" 
#include <memory>
#include <map>

#include "parameter.h"
#include "feature.h"
#include "rnnlm/rnnlmlib.h"


namespace Morph {
//TODO: ヘッダ間の依存関係の整理
class FeatureSet;
class FeatureTemplateSet;
class Node;
class NbestSearchToken; 
class TokenWithState;
class BeamQue;

struct morph_token_t {//{{{
	unsigned short lcAttr;
	unsigned short rcAttr;
	unsigned long posid; // id of part of speech
	unsigned long spos_id; // 細分類
	unsigned long form_id; // 活用形
	unsigned long form_type_id; // 活用型
	unsigned long base_id; // 活用型
    unsigned short length; // 単語の長さ
	short wcost; // cost of this morpheme
        
    unsigned long rep_id; // 代表表記
    unsigned long imis_id; // 意味情報
    unsigned long reading_id; // 読み
        
    //TODO: 連濁、文字正規化時のコストを素性化
    /* 小書き文字を大文字化、平仮名を長音記号に置換する際の追加コスト */
    //#define NORMALIZED_COST        6
    /* 長音を削除する際の追加コスト */
    //#define PROLONG_DEL_COST1      6  /* 感動詞 */
    //#define PROLONG_DEL_COST2     50  /* 判定詞 */
    //#define PROLONG_DEL_COST3      9  /* その他 (9より小さいと"あーあ"を指示詞と解析する) */
    /* 連濁認識用の追加コスト */
    //  #define VERB_VOICED_COST       7  /* "が"から始まる動詞を除く動詞の連濁化のコスト */
    //  #define VERB_GA_VOICED_COST    9  /* "が"から始まる動詞の連濁化のコスト */
    //  #define NOUN_VOICED_COST       8  /* "が"から始まる名詞を除く名詞の連濁化のコスト */
    //  #define NOUN_GA_VOICED_COST   11  /* "が"から始まる名詞の連濁化のコスト */
    //  #define ADJECTIVE_VOICED_COST  9  /* 形容詞の連濁化のコスト */
    //  #define OTHER_VOICED_COST      5  /* 上記以外の連濁化のコスト */
};//}}}
 
class TokenWithState{//{{{
    public:
        explicit TokenWithState(){};
        TokenWithState(const TokenWithState& tmp){//{{{
            if(tmp.f.get() != nullptr)
                f = std::make_unique<FeatureSet>(*(tmp.f)); //unique_ptr
            score = tmp.score;
            word_score = tmp.word_score;
            context_score = tmp.context_score;
            word_context_score = tmp.word_context_score;
            context = tmp.context;
            node_history = tmp.node_history;
        };//}}}
        TokenWithState& operator=(const TokenWithState& tmp){//{{{
            if(tmp.f.get() != nullptr)
                f = std::make_unique<FeatureSet>(*(tmp.f)); //unique_ptr
            score = tmp.score;
            word_score = tmp.word_score;
            context_score = tmp.context_score;
            word_context_score = tmp.word_context_score;
            context = tmp.context;
            node_history = tmp.node_history;
            return *this;
        };//}}}
        ~TokenWithState(){};
            
        double score = -DBL_MAX;
        double context_score = -DBL_MAX;
        double word_score = -DBL_MAX; //単語レベルのスコア
        double word_context_score = -DBL_MAX; //単語レベルのコンテクストスコア
        std::shared_ptr<RNNLM::context> context;
        std::vector<Node*> node_history;
        std::unique_ptr<FeatureSet> f;
            
        void init_feature(FeatureTemplateSet *ftmpl);
                    
        TokenWithState(Node* current_node, const TokenWithState &prev_token){
            node_history = prev_token.node_history; //copy
            node_history.emplace_back(current_node);
            f = std::make_unique<FeatureSet>(*(prev_token.f)); //copy
        };
            
//        void move_state_vector(std::vector<double> &&state){
//            context = std::move(state);
//        };
        //void set_history(std::vector<char> &&his){
        //    history = std::move(his);
        //};
}; //}}}

class BeamQue {//{{{
    private:
        unsigned int beam_width = 1;
    public:
        std::vector<TokenWithState> beam;
            
        BeamQue(){
            beam_width = 1;
            beam.resize(0);
        };

        BeamQue(unsigned int n){
            beam_width = n;
            beam.resize(0);
        };

        void setN(unsigned int n){
            beam_width = n;
            beam.resize(0);
        };

//        void push(TokenWithState tok){
//            // beam は昇順でソート済み
//            if( beam.size() == beam_width && beam.back().score > tok.score ){ //追加しない
//                return;
//            }else if(beam.size() == beam_width){//最小のものを置き換えて再ソート
//                beam.back() = std::move(std::move(tok));
//                std::sort(beam.begin(), beam.end(),[](auto x, auto y){return x.score + x.context_score> y.score + y.context_score ;});
//            }else{ //追加してリサイズ
//                beam.emplace_back(std::move(tok));
//                std::sort(beam.begin(), beam.end(),[](auto x, auto y){return x.score + x.context_score> y.score + y.context_score ;});
//                //std::sort(beam.begin(), beam.end(),[](auto x, auto y){return x.score > y.score;});
//                if(beam.size() > beam_width)
//                    beam.resize(beam_width);
//            }
//        }

        void push(TokenWithState&& tok){
            // beam は昇順でソート済み
            if( beam.size() == beam_width && (beam.back().score + beam.back().context_score > tok.score + tok.context_score )){ //追加しない
                return;
            }else if(beam.size() == beam_width){//最小のものを置き換えて再ソート
                beam.back() = (tok);
                std::sort(beam.begin(), beam.end(),[](auto x, auto y){return x.score + x.context_score > y.score + y.context_score ;});
            }else{ //追加してリサイズ
                beam.emplace_back((tok));
                std::sort(beam.begin(), beam.end(),[](auto x, auto y){return x.score + x.context_score > y.score + y.context_score ;});
                if(beam.size() > beam_width)
                    beam.resize(beam_width);
            }
        }
};//}}}

typedef struct morph_token_t Token;
    
class BigramInfo {//Second order Viterbi用  {{{
  public: long cost;
    long total_cost;
    FeatureSet *feature;
    Node *prev;
    BigramInfo() {
        cost = total_cost = 0;
        feature = NULL;
        prev = NULL;
    }
};//}}}

//TODO: posid を grammar と共通化し， ない場合にのみ新しいid を追加するようにする. 
//TODO: 巨大な定数 mapping も削除する
//TODO: ポインタ撲滅

class Node {//{{{
  private:
    static int id_count;

    //TODO: Topic 関係はあとで外に出してまとめる
    constexpr static const char* cdb_filename = "/home/morita/work/juman_LDA/dic/all_uniq.cdb";
    static DBM_FILE topic_cdb;
    static Parameter *param;

  public:
    bool init_bigram_info = false;
    Node *prev = nullptr; // best previous node determined by Viterbi algorithm
    Node *next = nullptr;
    Node *enext = nullptr; // next node that ends at this position
    Node *bnext = nullptr; // next node that begins at this position
    const char *surface = nullptr; 
    std::string *string = nullptr; // 未定義語の場合など，素性に使うため書き換える可能性がある
    std::string *string_for_print = nullptr;
    std::string *end_string = nullptr;
    std::string *original_surface = nullptr; // 元々現れたままの表層
    FeatureSet *feature = nullptr;
        
    std::string *representation = nullptr;
    std::string *semantic_feature = nullptr; 
        
//ifdef DEBUG
    std::map<std::string, std::string> debug_info; 
//endif

    // codes for second order bigram
    std::map<unsigned int, BigramInfo *> best_bigram;
    // std::vector<BigramInfo *> best_bigram;
    long get_bigram_cost(Node *left_node) {
        return best_bigram[left_node->id]->cost;
    }
    void set_bigram_cost(Node *left_node, long in_cost) {
        best_bigram[left_node->id]->cost = in_cost;
    }
    long get_bigram_total_cost(Node *left_node) {
        auto tmp = best_bigram[left_node->id];
        return tmp->total_cost;
    }
    void set_bigram_total_cost(Node *left_node, long in_cost) {
        best_bigram[left_node->id]->total_cost = in_cost;
    }
    Node *get_bigram_best_prev(Node *left_node) {
        return best_bigram[left_node->id]->prev;
    }
    void set_bigram_best_prev(Node *left_node, Node *best_l2_node) {
        best_bigram[left_node->id]->prev = best_l2_node;
        left_node->prev = best_l2_node;
    }
    FeatureSet *get_bigram_feature(Node *left_node) {
        return best_bigram[left_node->id]->feature;
    }
    ///void set_bigram_feature(Node *left_node, FeatureSet *in_feature) {
    //    best_bigram[left_node->id]->feature = in_feature;
    //}
    void append_bigram_feature(Node *left_node, FeatureSet *in_feature);
    void reserve_best_bigram(){};

    unsigned short length = 0; /* length of morph */
    unsigned short char_num = 0; //charlattice で指定する．TODO:コンストラクタ内で指定
    unsigned short rcAttr = 0;
    unsigned short lcAttr = 0;
	unsigned long posid = 0;
	unsigned long sposid = 0;
	unsigned long formid = 0;
	unsigned long formtypeid = 0;
	unsigned long baseid = 0;
	unsigned long repid = 0;
	unsigned long imisid = 0;
	unsigned long readingid = 0;
    std::string *pos = nullptr; //weak ptr
	std::string *spos = nullptr;
	std::string *form = nullptr;
	std::string *form_type = nullptr;
	std::string *base = nullptr;
    std::string *reading = nullptr;
    unsigned int char_type = 0; //先頭文字の文字タイプ
    unsigned int char_family = 0;
    unsigned int end_char_family = 0;
    unsigned char stat = 0; //TODO: どのような状態がありるうるのかを列挙
    bool used_in_nbest = false;
    bool longer = false;
    bool suuji = false;
    double wcost = 0; // score of this morpheme
    double cost = 0;  // score to this node (without context cost)
    double twcost = 0; // total score of this morpheme
    double tcost = 0; // total score to this node
    struct morph_token_t *token = nullptr;
    BeamQue bq;
                        
	//for N-best and Juman-style output
	int id = 1;
	unsigned int starting_pos = 0; // starting position
	std::priority_queue<unsigned int, std::vector<unsigned int>,
			std::greater<unsigned int> > connection; // id of previous nodes connected
	std::vector<NbestSearchToken> traceList; // keep track of n-best paths
         
    Node();
    //Node(const Node& node);
    ~Node();
         
    void print();
    std::string str(); 
    void clear();
    bool is_dummy();
    const char *get_first_char();
    unsigned short get_char_num();
        
    static void reset_id_count(){
        id_count = 1;
    };
    static Node make_dummy_node(){return Node();}

    static void set_param(Parameter *in_param){ param = in_param; };
                
    bool topic_available();
    bool topic_available_for_sentence();
    void read_vector(const char* buf, std::vector<double> &vector);
        
    TopicVector get_topic();
};//}}}

// obs
class NbestSearchToken {//{{{

public:
	double score = 0;
	Node* prevNode = nullptr; //access to previous trace-list
	int rank = 0; //specify an element in previous trace-list

	NbestSearchToken(Node* pN) {
		prevNode = pN;
	};

	NbestSearchToken(double s, int r) {
		score = s;
		rank = r;
	};

	NbestSearchToken(double s, int r, Node* pN) {
		score = s;
		rank = r;
		prevNode = pN;
	};

	~NbestSearchToken() {

	};

	bool operator<(const NbestSearchToken &right) const {
		if ((*this).score < right.score)
			return true;
		else
			return false;
	}

};//}}}


}

#endif

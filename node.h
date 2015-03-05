#ifndef NODE_H
#define NODE_H

#include "common.h"
#include "feature.h"
#include "pos.h"
extern "C"{
#include "cdb_juman.h"
}
#include "scw.h" 
#include <map>

namespace Morph {
class FeatureSet;
class NbestSearchToken; 

struct morph_token_t {//{{{
	unsigned short lcAttr;
	unsigned short rcAttr;
	unsigned short posid; // id of part of speech
	unsigned short spos_id; // 細分類
	unsigned short form_id; // 活用形
	unsigned short form_type_id; // 活用型
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
typedef struct morph_token_t Token;

//TODO: posid を grammar と共通化し， ない場合にのみ新しいid を追加するようにする. 
//TODO: 巨大な定数 mapping も削除する
//TODO: ポインタ撲滅
class Node {
  private:
    static int id_count;

    //TODO: Topic 関係はあとで外に出してまとめる
    constexpr static const char* cdb_filename = "/home/morita/work/juman_LDA/dic/all_uniq.cdb";
    static DBM_FILE topic_cdb;
  public:
    Node *prev = nullptr; // best previous node determined by Viterbi algorithm
    Node *next = nullptr;
    Node *enext = nullptr; // next node that ends at this position
    Node *bnext = nullptr; // next node that begins at this position
    const char *surface = nullptr; 
    std::string *original_surface = nullptr; // 元々現れたままの表層
    std::string *string = nullptr; // 未定義語の場合など，素性に使うため書き換える可能性がある
    std::string *string_for_print = nullptr;
    std::string *end_string = nullptr;
    FeatureSet *feature = nullptr;
        
    std::string *representation = nullptr;
    std::string *semantic_feature = nullptr; 
        
//ifdef DEBUG
    std::map<std::string, std::string> debug_info; 
//endif
        
    unsigned short length = 0; /* length of morph */
    unsigned short char_num = 0;
    unsigned short rcAttr = 0;
    unsigned short lcAttr = 0;
	unsigned short posid = 0;
	unsigned short sposid = 0;
	unsigned short formid = 0;
	unsigned short formtypeid = 0;
	unsigned long baseid = 0;
	unsigned long repid = 0;
	unsigned long imisid = 0;
	unsigned long readingid = 0;
    std::string *pos = nullptr;
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
    double wcost = 0; // cost of this morpheme
    double cost = 0; // total cost to this node
    struct morph_token_t *token = nullptr;
        
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
    void clear();
    bool is_dummy();
    const char *get_first_char();
    unsigned short get_char_num();
        
    static void reset_id_count(){
        id_count = 1;
    };
    static Node make_dummy_node(){return Node();}
        
    bool topic_available();
    bool topic_available_for_sentence();
    void read_vector(const char* buf, std::vector<double> &vector);

    TopicVector get_topic();

};

class NbestSearchToken {

public:
	double score = 0;
	Node* prevNode = nullptr; //access to previous trace-list
	int rank = 0; //specify an element in previous trace-list

	NbestSearchToken(Node* pN) {
		prevNode = pN;
	}
	;

	NbestSearchToken(double s, int r) {
		score = s;
		rank = r;
	}
	;

	NbestSearchToken(double s, int r, Node* pN) {
		score = s;
		rank = r;
		prevNode = pN;
	}
	;

	~NbestSearchToken() {

	}
	;

	bool operator<(const NbestSearchToken &right) const {
		if ((*this).score < right.score)
			return true;
		else
			return false;
	}

};
}

#endif

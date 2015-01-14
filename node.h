#ifndef NODE_H
#define NODE_H

#include "common.h"
#include "feature.h"

namespace Morph {
class FeatureSet;
class NbestSearchToken; 

struct morph_token_t {
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

	// unsigned int feature;
	// unsigned int compound;  /* reserved for noun compound */
};
typedef struct morph_token_t Token;

class Node {
  public:
    Node *prev; // best previous node determined by Viterbi algorithm
    Node *next;
    Node *enext; // next node that ends at this position
    Node *bnext; // next node that begins at this position
    // struct morph_path_t  *rpath;
    // struct morph_path_t  *lpath;
    // struct morph_node_t **begin_node_list;
    // struct morph_node_t **end_node_list;
    const char *surface;
    std::string *string;
    std::string *string_for_print;
    std::string *end_string;
    FeatureSet *feature;
        
    std::string *representation;
    std::string *semantic_feature; 
    std::string *original_surface;
        
    // const char *feature;
    // unsigned int id;
    unsigned short length; /* length of morph */
    unsigned short char_num;
    unsigned short rcAttr;
    unsigned short lcAttr;
	unsigned short posid;
	unsigned short sposid;
	unsigned short formid;
	unsigned short formtypeid;
	unsigned long baseid;
	unsigned long repid;
	unsigned long imisid;
	unsigned long readingid;
    std::string *pos;
	std::string *spos;
	std::string *form;
	std::string *form_type;
	std::string *base;
    std::string *reading;
    unsigned int char_type;
    unsigned int char_family;
    unsigned int end_char_family;
    unsigned char stat;
    bool used_in_nbest;
    short wcost; // cost of this morpheme
    long cost; // total cost to this node
    struct morph_token_t *token;

	//for N-best and Juman-style output
	unsigned int id;
	unsigned int starting_pos; // starting position
	std::priority_queue<unsigned int, std::vector<unsigned int>,
			std::greater<unsigned int> > connection; // id of previous nodes connected
	std::vector<NbestSearchToken> traceList; // keep track of n-best paths
    
    Node();
    ~Node();
    void print();
    const char *get_first_char();
    unsigned short get_char_num();
};

//shen版からのコピー
class NbestSearchToken {

public:
	long score;
	Node* prevNode; //access to previous trace-list
	int rank; //specify an element in previous trace-list

	NbestSearchToken(Node* pN) {
		prevNode = pN;
	}
	;

	NbestSearchToken(long s, int r) {
		score = s;
		rank = r;
	}
	;

	NbestSearchToken(long s, int r, Node* pN) {
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

#ifndef NODE_H
#define NODE_H

#include "common.h"
#include "feature.h"

namespace Morph {
class FeatureSet;

struct morph_token_t {
	unsigned short lcAttr;
	unsigned short rcAttr;
	unsigned short posid; // id of part of speech
	unsigned short spos_id; // 細分類
	unsigned short form_id; // 活用形
	unsigned short form_type_id; // 活用型
	unsigned long base_id; // 活用型
	short wcost; // cost of this morpheme
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
    std::string *pos;
	std::string *spos;
	std::string *form;
	std::string *form_type;
	std::string *base;
    unsigned int char_type;
    unsigned int char_family;
    unsigned int end_char_family;
    unsigned char stat;
    short wcost; // cost of this morpheme
    long cost; // total cost to this node
    struct morph_token_t *token;
    Node();
    ~Node();
    void print();
    const char *get_first_char();
    unsigned short get_char_num();
};

}

#endif

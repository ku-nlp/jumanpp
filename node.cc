#include "common.h"
#include "node.h"

namespace Morph {

int Node::id_count = 0;

Node::Node() {
    //memset(this, 0, sizeof(Node));
    this->id = id_count++;
}

Node::Node(const Node& node) { // 普通のコピー
    //memset(this, 0, sizeof(Node));
    
    this->prev = node.prev;
    this->next = node.next;
    this->enext = node.enext;
    this->bnext = node.bnext;
    this->surface = node.surface;
    this->string_for_print = node.string_for_print;
    this->end_string = node.end_string;
    this->representation = node.representation;
    this->semantic_feature = node.semantic_feature; 
    this->debug_info = node.debug_info;
    this->length = node.length; /* length of morph */
    this->char_num = node.char_num;
    this->rcAttr = node.rcAttr;
    this->lcAttr = node.lcAttr;
	this->posid = node.posid;
	this->sposid = node.sposid;
	this->formid = node.formid;
	this->formtypeid = node.formtypeid;
	this->baseid = node.baseid;
	this->repid = node.repid;
	this->imisid = node.imisid;
	this->readingid = node.readingid;
    this->pos = node.pos;
	this->spos = node.spos;
	this->form = node.form;
	this->form_type = node.form_type;
	this->base = node.base;
    this->reading = node.reading;
    this->char_type = node.char_type;
    this->char_family = node.char_family;
    this->end_char_family = node.end_char_family;
    this->stat = node.stat;
    this->used_in_nbest = node.used_in_nbest;
    this->wcost = node.wcost;
    this->cost = node.cost;
    this->token = node.token;

	//for N-best and Juman-style output
	this->id = node.id;
	this->starting_pos = node.starting_pos; 

    if (node.string_for_print != node.string && node.string_for_print)
        this->string_for_print = new std::string(* node.string_for_print);
    if (node.end_string != node.string && node.end_string)
        this->end_string = new std::string(* node.end_string);
    if (node.string && node.string)
        this->string = new std::string(* node.string);
    if (node.original_surface && node.string)
        this->original_surface = new std::string(* node.original_surface);
    if (node.feature)
        this->feature = new FeatureSet(*(node.feature));
}

Node::~Node() {
    if (string_for_print != string)
        delete string_for_print;
    if (end_string != string)
        delete end_string;
    if (string)
        delete string;
    if (original_surface)
        delete original_surface;
    if (feature)
        delete feature;
//    if (pos)
//        delete pos;
//    if (spos)
//        delete spos;
//    if (form)
//        delete form;
//    if (form_type)
//        delete form_type;
//    if (base)
//        delete base;
}

void Node::print() {
    cout << *(string_for_print) << "_" << *pos << ":" << *spos;
}

const char *Node::get_first_char() {
    return string_for_print->c_str();
}

unsigned short Node::get_char_num() {
    if (char_num >= MAX_RESOLVED_CHAR_NUM)
        return MAX_RESOLVED_CHAR_NUM;
    else
        return char_num;
}

}

#include "common.h"
#include "node.h"

namespace Morph {

Node::Node() {
    memset(this, 0, sizeof(Node));
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
    cout << *(string_for_print) << "_" << *pos;
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

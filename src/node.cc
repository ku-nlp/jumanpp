#include "common.h"
#include "feature.h"
#include "node.h"

// topic 関係の読込を無効化する
#define NO_USE_TOPIC_DB

namespace Morph {

int Node::id_count = 1;
Parameter *Node::param;

const char *hukugouji[] = {
    "めぐる",   "除く",     "のぞく",   "通じる",   "通ずる",   "通す",
    "含める",   "ふくめる", "始める",   "はじめる", "絡む",     "からむ",
    "沿う",     "そう",     "向ける",   "伴う",     "ともなう", "基づく",
    "よる",     "対する",   "関する",   "代わる",   "おく",     "つく",
    "とる",     "加える",   "くわえる", "限る",     "続く",     "つづく",
    "合わせる", "あわせる", "比べる",   "くらべる", "並ぶ",     "ならぶ",
    "おける",   "いう",     "する"};

Node::Node() : bq(param->N) { //{{{
    this->id = id_count++;
#ifndef NO_USE_TOPIC_DB
    if (topic_cdb == nullptr) {
        topic_cdb = db_read_open(cdb_filename);
    }
#endif
} //}}}

bool Node::is_dummy() { //{{{
    return (stat == MORPH_DUMMY_NODE || stat == MORPH_BOS_NODE ||
            stat == MORPH_EOS_NODE);
}; //}}}

Node::~Node() { //{{{
    if (string) {
        delete string;
        string = nullptr;
    }
    if (string_for_print) {
        delete string_for_print;
        string_for_print = nullptr;
    }
    if (end_string) {
        delete end_string;
        end_string = nullptr;
    }
    if (original_surface) {
        delete original_surface;
        original_surface = nullptr;
    }
    if (feature) {
        delete feature;
        feature = nullptr;
    }
} //}}}

void TokenWithState::init_feature(FeatureTemplateSet *ftmpl) { //{{{
    f = std::unique_ptr<FeatureSet>(new FeatureSet(ftmpl));
}; //}}}

void Node::clear() { //{{{
    if (string)
        delete string;
    if (string_for_print)
        delete string_for_print;
    if (end_string)
        delete end_string;
    if (original_surface)
        delete original_surface;
    if (feature)
        delete feature;
    //*this = Node();
} //}}}

void Node::print() { //{{{
    cout << *(string_for_print) << "_" << *pos << ":" << *spos;
} //}}}

std::string Node::str() { //{{{
    return *(string_for_print) + "_" + *pos + ":" + *spos;
} //}}}

const char *Node::get_first_char() { //{{{
    return string_for_print->c_str();
} //}}}

unsigned short Node::get_char_num() { //{{{
    if (char_num >= MAX_RESOLVED_CHAR_NUM)
        return MAX_RESOLVED_CHAR_NUM;
    else
        return char_num;
} //}}}
}

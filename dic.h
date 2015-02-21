#ifndef DIC_H
#define DIC_H

#include "common.h"
#include "darts.h"
#include "mmap.h"
#include "pos.h"
#include "node.h"
#include "feature.h"
#include "parameter.h"
#include "u8string.h"

namespace Morph {

class Dic {
    Parameter *param;
    Darts::DoubleArray darts;
    Mmap<char> *dmmap;
    const Token *token_head;
    Pos posid2pos;
    Pos sposid2spos;
    Pos formid2form;
    Pos formtypeid2formtype;
    Pos baseid2base;
    Pos readingid2reading;
    Pos repid2rep;
    Pos imisid2imis;
    FeatureTemplateSet *ftmpl;
    static constexpr char* DEF_ONOMATOPOEIA_HINSI = "副詞";
    static constexpr char* DEF_ONOMATOPOEIA_BUNRUI = "*";
    static constexpr char* DEF_ONOMATOPOEIA_IMIS = "自動認識";
  public:
    static const std::unordered_map<std::string,int> pos_map;
    static const std::unordered_map<std::string,int> spos_map;
    static const std::unordered_map<std::string,int> katuyou_type_map;
    static const std::unordered_map<std::string,int> katuyou_form_map;

    Dic() { posid2pos.get_id(BOS); posid2pos.get_id(EOS);}
    Dic(Parameter *in_param, FeatureTemplateSet *in_ftmpl);
    ~Dic();

    bool open(Parameter *in_param, FeatureTemplateSet *in_ftmpl);
    Node *lookup(const char *start_str, unsigned int specified_length, std::string *specified_pos);
    Node *lookup_specified(const char *start_str, unsigned int specified_length, const std::vector<std::string>& specified_pos);

    Node *lookup_lattice(std::vector<Darts::DoubleArray::result_pair_type> &da_search_result, const char *start_str);
    Node *lookup_lattice(std::vector<Darts::DoubleArray::result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length, std::string *specified_pos); 
    Node *lookup_lattice(std::vector<Darts::DoubleArray::result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length, unsigned short specified_posid);
    Node* recognize_onomatopoeia(const char* start_str);
             
    Node *make_unk_pseudo_node(const char *start_str, int byte_len);
    Node *make_unk_pseudo_node(const char *start_str, int byte_len, const std::string &specified_pos);
    Node *make_unk_pseudo_node(const char *start_str, int byte_len, unsigned short specified_posid);
    
    // 暫定。あとでオプションとして統合する
    Node *make_unk_pseudo_node_gold(const char *start_str, int byte_len, std::string &specified_pos);

    Node *make_unk_pseudo_node_list_some_pos(const char *start_str, int byte_len, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos);
    Node *make_unk_pseudo_node_list_some_pos_by_dic_check(const char *start_str, int byte_len, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos, Node* r_node);

    Node *make_unk_pseudo_node_list(const char *start_str, unsigned int min_char_num, unsigned int max_char_num);
    Node *make_unk_pseudo_node_list(const char *start_str, unsigned int min_char_num, unsigned int max_char_num, unsigned short specified_posid);

    Node *make_specified_pseudo_node(const char *start_str, unsigned int specified_length, std::string *specified_pos, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family);
    Node *make_specified_pseudo_node(const char *start_str, unsigned int specified_length, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family);

    Node *make_specified_pseudo_node_by_dic_check(const char *start_str, unsigned int specified_length, std::string *inspecified_pos, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family, Node* r_node);

    const Token *get_token(const Darts::DoubleArray::result_pair_type &n) const {
        return token_head + (n.value >> 8);
    }

    size_t token_size(const Darts::DoubleArray::result_pair_type &n) const { return 0xff & n.value; }

    void inline read_node_info(const Token &token, Node **node);
    friend class Sentence;
};

#define MMAP_OPEN(type, map, file) do {         \
    map = new Mmap<type>;                       \
    if (!map->open(file)) {                     \
      return false;                             \
    }                                           \
  } while (0)

#define MMAP_CLOSE(type, map) do {              \
    delete map; } while (0)
}

#endif

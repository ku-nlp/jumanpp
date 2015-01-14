#include "common.h"
#include "dic.h"

namespace Morph {

Dic::~Dic() {
    MMAP_CLOSE(char, dmmap);
    darts.clear();
}

Dic::Dic(Parameter *in_param, FeatureTemplateSet *in_ftmpl) {
    open(in_param, in_ftmpl);
}

bool Dic::open(Parameter *in_param, FeatureTemplateSet *in_ftmpl) {
    param = in_param;
    ftmpl = in_ftmpl;

    if (darts.open(param->darts_filename.c_str()) == -1) {
        cerr << ";; Cannot open: " << param->darts_filename << endl;
        return false;
    }else{
        cerr << ";; " << param->darts_filename  << endl;
    }
        
    MMAP_OPEN(char, dmmap, param->dic_filename.c_str());
    const char *ptr = dmmap->begin();
    token_head = reinterpret_cast<const Token *>(ptr);
        
    posid2pos.read_pos_list(param->pos_filename);
    sposid2spos.read_pos_list(param->spos_filename);
    formid2form.read_pos_list(param->form_filename);
    formtypeid2formtype.read_pos_list(param->form_type_filename);
    baseid2base.read_pos_list(param->base_filename);

    std::vector<std::string> c;
    split_string(UNK_POSS, ",", c);
    for (std::vector<std::string>::iterator it = c.begin(); it != c.end(); it++) {
        param->unk_pos.push_back(posid2pos.get_id(*it));
    }
        
    split_string(UNK_FIGURE_POSS, ",", c);
    for (std::vector<std::string>::iterator it = c.begin(); it != c.end(); it++){
        param->unk_figure_pos.push_back(posid2pos.get_id(*it));
    }

    return true;
}

Node *Dic::lookup(const char *start_str) {
    return lookup(start_str, 0, MORPH_DUMMY_POS);
}

Node *Dic::lookup(const char *start_str, unsigned int specified_length, std::string *specified_pos) {
    if (specified_pos)
        return lookup(start_str, specified_length, posid2pos.get_id(*specified_pos));
    else
        return lookup(start_str, specified_length, MORPH_DUMMY_POS);
}

Node *Dic::lookup(const char *start_str, unsigned int specified_length, unsigned short specified_posid) {//{{{
    Node *result_node = NULL;

    // search double array
    Darts::DoubleArray::result_pair_type result_pair[1024];
    size_t num = darts.commonPrefixSearch(start_str, result_pair, 1024);
    if (num == 0)
        return result_node;

    for (size_t i = 0; i < num; i++) { // hit num
        if (specified_length && specified_length != result_pair[i].length)
            continue;
        size_t size  = token_size(result_pair[i]);
        const Token *token = get_token(result_pair[i]);
        for (size_t j = 0; j < size; j++) { // same key but different value (pos)
            if (specified_posid != MORPH_DUMMY_POS && specified_posid != (token + j)->posid)
                continue;
            Node *new_node = new Node;
            read_node_info(*(token + j), &new_node);
            new_node->token = (Token *)(token + j);
            new_node->length = result_pair[i].length;
            new_node->surface = start_str;
            new_node->char_num = utf8_chars((unsigned char *)start_str, new_node->length);
            new_node->string_for_print = new std::string(start_str, new_node->length);
            if (new_node->lcAttr == 1) { // Wikipedia
                new_node->string = new std::string(UNK_WIKIPEDIA);
                new_node->stat = MORPH_UNK_NODE;
            } else {
                new_node->string = new_node->string_for_print;
                new_node->stat = MORPH_NORMAL_NODE;
            }
            new_node->char_type = check_utf8_char_type((unsigned char *)start_str);
            new_node->char_family = check_char_family(new_node->char_type);
            char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
            new_node->end_char_family = check_char_family((unsigned char *)end_char);
            new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));

            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_unigram_feature(new_node);
            new_node->wcost = f->calc_inner_product_with_weight();
            new_node->feature = f;

            new_node->bnext = result_node;
            result_node = new_node;
        }
    }
    return result_node;
}//}}}

Node *Dic::lookup_lattice(std::vector<Darts::DoubleArray::result_pair_type> &da_search_result,const char *start_str) {
    return lookup_lattice(da_search_result, start_str, 0, MORPH_DUMMY_POS);
}

Node *Dic::lookup_lattice(std::vector<Darts::DoubleArray::result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length, std::string *specified_pos) {
    if (specified_pos)
        return lookup_lattice( da_search_result,start_str, specified_length, posid2pos.get_id(*specified_pos));
    else
        return lookup_lattice( da_search_result,start_str, specified_length, MORPH_DUMMY_POS);
}

// DA から検索した結果を Node に変換する
// ラティスを受け取る方法を考える
Node *Dic::lookup_lattice(std::vector<Darts::DoubleArray::result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length, unsigned short specified_posid) { //{{{
    Node *result_node = NULL;
        
    // コレを受け取る様にしたほうが依存関係がスッキリする... と思いきや，darts を持ってるのはDic クラスだった
    // auto result_pair = da_search_from_position(int position);
    std::vector<Darts::DoubleArray::result_pair_type> &result_pair = da_search_result; 
        
    if (result_pair.size() == 0)
        return result_node;
    size_t num = result_pair.size();
        
    //以降のコードは完全に同じ？
    for (size_t i = 0; i < num; i++) { // hit num
        if (specified_length && specified_length != result_pair[i].length)
            continue;
        size_t size  = token_size(result_pair[i]);
        const Token *token = get_token(result_pair[i]);
            
        for (size_t j = 0; j < size; j++) { // same key but different value (pos)
            if (specified_posid != MORPH_DUMMY_POS && specified_posid != (token + j)->posid)
                continue;
            Node *new_node = new Node;
            read_node_info(*(token + j), &new_node);
            new_node->token = (Token *)(token + j);
            new_node->length = result_pair[i].length;// ここは変えるべき？
            new_node->surface = start_str;
            new_node->char_num = utf8_chars((unsigned char *)start_str, new_node->length);
            new_node->string_for_print = new std::string(start_str, new_node->length);
            if (new_node->lcAttr == 1) { // Wikipedia
                new_node->string = new std::string(UNK_WIKIPEDIA);
                new_node->stat = MORPH_UNK_NODE;
            } else {
                new_node->string = new_node->string_for_print;
                new_node->stat = MORPH_NORMAL_NODE;
            }
            new_node->char_type = check_utf8_char_type((unsigned char *)start_str);
            new_node->char_family = check_char_family(new_node->char_type);
            char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
            new_node->end_char_family = check_char_family((unsigned char *)end_char);
            new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
                
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_unigram_feature(new_node);
            new_node->wcost = f->calc_inner_product_with_weight();
            new_node->feature = f;
                
            new_node->bnext = result_node;
            result_node = new_node;
        }
    }
    return result_node;
}//}}}


Node *Dic::make_unk_pseudo_node(const char *start_str, int byte_len) {
    return make_unk_pseudo_node(start_str, byte_len, MORPH_DUMMY_POS);
}

Node *Dic::make_unk_pseudo_node(const char *start_str, int byte_len, std::string &specified_pos) {
    return make_unk_pseudo_node(start_str, byte_len, posid2pos.get_id(specified_pos));
}

// あとでオプションに変更する
// make an unknown word node
Node *Dic::make_unk_pseudo_node_gold(const char *start_str, int byte_len, std::string &specified_pos) {
    unsigned short specified_posid = posid2pos.get_id(specified_pos);

    Node *new_node = new Node;
    new_node->surface = start_str;
    new_node->length = byte_len;
    new_node->char_type = check_utf8_char_type((unsigned char *)new_node->surface);
    new_node->char_family = check_char_family(new_node->char_type);
    new_node->string = new std::string(new_node->surface, new_node->length);
    new_node->string_for_print = new std::string(new_node->surface, new_node->length);
    new_node->char_num = utf8_chars((unsigned char *)(new_node->surface), new_node->length);
    char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
    new_node->end_char_family = check_char_family((unsigned char *)end_char);
    new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
    new_node->stat = MORPH_UNK_NODE;
    if (specified_posid == MORPH_DUMMY_POS) {//ここは来ないはず
        cerr << ";; error there are unknown words on gold data" << endl;
    } else {
        new_node->posid  = specified_posid;
        new_node->sposid = sposid2spos.get_id(UNK_POS);
        new_node->formid = formid2form.get_id(UNK_POS);
        new_node->formtypeid = formtypeid2formtype.get_id(UNK_POS);
        new_node->baseid = baseid2base.get_id(UNK_POS);
    }
    new_node->pos = posid2pos.get_pos(new_node->posid);
    new_node->spos = sposid2spos.get_pos(new_node->sposid);
    new_node->form = formid2form.get_pos(new_node->formid);
    new_node->form_type = formtypeid2formtype.get_pos(new_node->formtypeid);
    new_node->base = baseid2base.get_pos(new_node->baseid);

    new_node->feature = new FeatureSet(ftmpl);
    new_node->feature->extract_unigram_feature(new_node);
    new_node->wcost = new_node->feature->calc_inner_product_with_weight();
    return new_node;
}

// make an unknown word node
Node *Dic::make_unk_pseudo_node(const char *start_str, int byte_len, unsigned short specified_posid) {
    Node *new_node = new Node;
    new_node->surface = start_str;
    new_node->length = byte_len;
    new_node->char_type = check_utf8_char_type((unsigned char *)new_node->surface);
    new_node->char_family = check_char_family(new_node->char_type);

    // 未知語処理をする場合も，ラベルとしては未定義アルファベット，未定義漢字，未定義カタカナ語などのラベルを付けるべき．
    if ((new_node->char_type == (TYPE_FIGURE||TYPE_KANJI_FIGURE)) && (specified_posid == posid2pos.get_id("名詞"))){
        new_node->string = new std::string("<数詞>");
        new_node->base = new std::string("<数詞>");
    } else if (new_node->char_type == TYPE_KANJI){
        new_node->string = new std::string("未定義漢字");
        new_node->base = new std::string("未定義漢字");
    } else if (new_node->char_type == TYPE_KATAKANA){
        new_node->string = new std::string("未定義カタカナ語");
        new_node->base = new std::string("未定義カタカナ語");
    } else if (new_node->char_type == TYPE_HIRAGANA){
        new_node->string = new std::string("未定義ひらがな語");
        new_node->base = new std::string("未定義ひらがな語");
    } else {//何が来る？記号？
        new_node->string = new std::string(new_node->surface, new_node->length);
        new_node->base = new std::string("未定義文字種不明");
    } //漢字かな交じりはどこに入る？
      
    new_node->string_for_print = new std::string(new_node->surface, new_node->length);
    new_node->char_num = utf8_chars((unsigned char *)(new_node->surface), new_node->length);
    char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
    new_node->end_char_family = check_char_family((unsigned char *)end_char);
    new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
    new_node->stat = MORPH_UNK_NODE;
        
    // pos が指定されている場合とそうでない場合で分岐する
    // 品詞などを埋める
    if (specified_posid == MORPH_DUMMY_POS) {
        new_node->posid  = specified_posid;
        new_node->pos = posid2pos.get_pos(new_node->posid);
        new_node->sposid = sposid2spos.get_id(UNK_POS);
        new_node->spos = sposid2spos.get_pos(new_node->sposid);
        new_node->formid = formid2form.get_id(UNK);
        new_node->formtypeid = formtypeid2formtype.get_id(UNK);
        if ((new_node->char_type == (TYPE_FIGURE)) || (new_node->char_type == (TYPE_KANJI_FIGURE))){
            //数詞については詳細品詞まで分かるので入れておく
            new_node->string = new std::string("<数詞>");
            new_node->posid = posid2pos.get_id("名詞");
            new_node->pos = new std::string("名詞");
            new_node->sposid = sposid2spos.get_id("数詞");
            new_node->spos = new std::string("数詞");
            new_node->formid = formid2form.get_id("数詞");
            new_node->form = new std::string("*");
            new_node->formtypeid = formtypeid2formtype.get_id("*");
            new_node->base = new std::string("<数詞>");
        } 
    } else {//品詞が指定されている場合
        if ((new_node->char_type == (TYPE_FIGURE || TYPE_KANJI_FIGURE)) && specified_posid == posid2pos.get_id("名詞") ){
            //数詞については詳細品詞まで分かるので入れておく
            new_node->string = new std::string("<数詞>");
            new_node->posid = posid2pos.get_id("名詞");
            new_node->pos = new std::string("名詞");
            new_node->sposid = sposid2spos.get_id("数詞");
            new_node->spos = new std::string("数詞");
            new_node->formid = formid2form.get_id("*");
            new_node->formtypeid = formtypeid2formtype.get_id("*");
            new_node->base = new std::string("<数詞>");
        }else{
            new_node->posid  = specified_posid;
            new_node->pos = posid2pos.get_pos(new_node->posid);
            new_node->sposid = sposid2spos.get_id(UNK_POS);
            new_node->spos = sposid2spos.get_pos(new_node->sposid);
            new_node->formid = formid2form.get_id(UNK_POS);
            new_node->formtypeid = formtypeid2formtype.get_id(UNK_POS);
        }
    }
    new_node->form = formid2form.get_pos(new_node->formid);
    new_node->form_type = formtypeid2formtype.get_pos(new_node->formtypeid);
            
    new_node->feature = new FeatureSet(ftmpl);
    new_node->feature->extract_unigram_feature(new_node);
    new_node->wcost = new_node->feature->calc_inner_product_with_weight();
    // new_node->wcost = MORPH_UNK_COST;
    return new_node;
}

Node *Dic::make_unk_pseudo_node_list_some_pos_by_dic_check(const char *start_str, int byte_len, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos, Node* r_node) {
    Node *result_node = NULL;
    if (specified_posid == MORPH_DUMMY_POS ) {
        for (std::vector<unsigned short>::iterator it = specified_unk_pos->begin(); it != specified_unk_pos->end(); it++) {
            Node *new_node = make_unk_pseudo_node(start_str, byte_len, *it);

            bool flag_covered = false;
            Node *tmp_node = r_node;
            if(r_node)
                while (tmp_node->bnext){
                    if(tmp_node->length == new_node->length &&
                            tmp_node->posid == new_node->posid)
                        flag_covered = true;
                    tmp_node = tmp_node->bnext;
                } //辞書チェック

            if(!flag_covered){
                new_node->bnext = result_node;
                result_node = new_node;
            }
        }
    }else{ // POSが分かる場合(主に訓練時)
        Node *new_node = make_unk_pseudo_node(start_str, byte_len, specified_posid);

        bool flag_covered = false;
        Node *tmp_node = r_node;
        //辞書チェック
        while (tmp_node){
            if(tmp_node->length == new_node->length &&
                    tmp_node->posid == specified_posid){
                flag_covered = true;
            }
            tmp_node = tmp_node->bnext;
        } 
        if(!flag_covered){
            new_node->bnext = result_node;
            result_node = new_node;
        }
    }
    return result_node;
}

Node *Dic::make_unk_pseudo_node_list_some_pos(const char *start_str, int byte_len, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos) {
    Node *result_node = NULL;
        
    if (specified_posid == MORPH_DUMMY_POS) {
        // POSが分からない場合(テスト時)
        // 未知語の候補POSすべてを生成
        for (std::vector<unsigned short>::iterator it = specified_unk_pos->begin(); it != specified_unk_pos->end(); it++) {
            Node *new_node = make_unk_pseudo_node(start_str, byte_len, *it);
            new_node->bnext = result_node;
            result_node = new_node;
        }
    } else {
        // POSが分かる場合(主に訓練時)
        Node *new_node = make_unk_pseudo_node(start_str, byte_len, specified_posid);
        new_node->bnext = result_node;
        result_node = new_node;
    }
    return result_node;
}

Node *Dic::make_unk_pseudo_node_list(const char *start_str, unsigned int min_char_num, unsigned int max_char_num) {
    return make_unk_pseudo_node_list(start_str, min_char_num, max_char_num, MORPH_DUMMY_POS);
}

// make unknown word nodes of some lengths
Node *Dic::make_unk_pseudo_node_list(const char *start_str, unsigned int min_char_num, unsigned int max_char_num, unsigned short specified_posid) {
    Node *result_node = NULL;
    unsigned int length = strlen(start_str), char_num = 0;
    for (unsigned int pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(start_str + pos))) {
        // figures and alphabets that are processed separately
        if (compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FIGURE | TYPE_KANJI_FIGURE | TYPE_ALPH))
            break;
        // stop if it's a punctuation or a space
        else if (pos > 0 && compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FAMILY_PUNC | TYPE_FAMILY_SPACE))
            break;
        else if (char_num >= max_char_num) // max characters as <UNK>
            break;
        else if (char_num < min_char_num - 1) { // skip while the length is shorter than the specified length
            char_num++;
            continue;
        }
        Node *new_node = make_unk_pseudo_node_list_some_pos(start_str, pos + utf8_bytes((unsigned char *)(start_str + pos)), specified_posid, &(param->unk_pos));
        Node *tmp_node = new_node;
        while (tmp_node->bnext)
            tmp_node = tmp_node->bnext;
        tmp_node->bnext = result_node;
        result_node = new_node;
        char_num++;
    }
    return result_node;
}

Node *Dic::make_specified_pseudo_node(const char *start_str, unsigned int specified_length, std::string *specified_pos, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family) {
    if (specified_pos)
        return make_specified_pseudo_node(start_str, specified_length, posid2pos.get_id(*specified_pos), specified_unk_pos, type_family);
    else
        return make_specified_pseudo_node(start_str, specified_length, MORPH_DUMMY_POS, specified_unk_pos, type_family);
}

Node *Dic::make_specified_pseudo_node_by_dic_check(const char *start_str, unsigned int specified_length, std::string *specified_pos, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family, Node* r_node) {
    unsigned short specified_posid = MORPH_DUMMY_POS;
    if(specified_pos) specified_posid = posid2pos.get_id(*specified_pos);

    unsigned int length = strlen(start_str);
    unsigned int pos = 0, char_num = 0;
    for (pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(start_str + pos))) {
        // exceptional figure expression of two characters
        // 数字を指定していて、かつ数字の例外に当たる場合
        if ((type_family == TYPE_FAMILY_FIGURE && check_exceptional_two_chars_in_figure((start_str + pos), length - pos))) {
            pos += utf8_bytes((unsigned char *)(start_str + pos));
            char_num += 2;
        }
        // doesn't start with slash, colon, etc.
        else if (pos == 0 && compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FAMILY_PUNC_SYMBOL))
            break;
        else if (compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), type_family)) // this is in specified family
            char_num++;
        else
            break;
    }

    if (char_num > 0 && (!specified_length || specified_length == pos)) {

        Node *new_node = make_unk_pseudo_node_list_some_pos_by_dic_check(start_str, pos, specified_posid, specified_unk_pos, r_node); // pos == byte_len
        // cerr << "CAND:" << *(new_node->string_for_print) << endl;
        return new_node;
    }
    else {
        return NULL;
    }

}

// make figure nodes
Node *Dic::make_specified_pseudo_node(const char *start_str, unsigned int specified_length, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family) {
    unsigned int length = strlen(start_str);
    unsigned int pos = 0, char_num = 0;
    for (pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(start_str + pos))) {
        // exceptional figure expression of two characters
        if ((type_family == TYPE_FAMILY_FIGURE && check_exceptional_two_chars_in_figure((start_str + pos), length - pos))) {
            pos += utf8_bytes((unsigned char *)(start_str + pos));
            char_num += 2;
        }
        // doesn't start with slash, colon, etc.
        else if (pos == 0 && compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FAMILY_PUNC_SYMBOL))
            break;
        else if (compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), type_family)) // this is in specified family
            char_num++;
        else
            break;
    }

    if (char_num > 0 && 
        (!specified_length || specified_length == pos)) {
        Node *new_node = make_unk_pseudo_node_list_some_pos(start_str, pos, specified_posid, specified_unk_pos); // pos == byte_len
        // cerr << "CAND:" << *(new_node->string_for_print) << endl;
        return new_node;
    }
    else {
        return NULL;
    }
}

// mkdarts => Dic
void inline Dic::read_node_info(const Token &token, Node **node) {
    (*node)->lcAttr = token.lcAttr;
    (*node)->rcAttr = token.rcAttr;

    (*node)->posid = token.posid;
    (*node)->sposid = token.spos_id;
    (*node)->formid = token.form_id;
    (*node)->formtypeid = token.form_type_id;
    (*node)->baseid = token.base_id;

    (*node)->pos = posid2pos.get_pos((*node)->posid); 
    //cout << "p" << *(*node)->pos << " " ;
    (*node)->spos = sposid2spos.get_pos((*node)->sposid); 
    (*node)->form = formid2form.get_pos((*node)->formid);
    (*node)->form_type = formtypeid2formtype.get_pos((*node)->formtypeid);
    (*node)->base = (std::string *)baseid2base.get_pos((*node)->baseid);

    (*node)->wcost = token.wcost;
    (*node)->token = const_cast<Token *>(&token);
}

}


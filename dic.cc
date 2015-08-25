#include "common.h"
#include "feature.h"
#include "dic.h"

namespace Morph {

Dic::~Dic() {//{{{
    MMAP_CLOSE(char, dmmap);
    darts.clear();
}//}}}

Dic::Dic(Parameter *in_param, FeatureTemplateSet *in_ftmpl) {//{{{
    open(in_param, in_ftmpl);
}//}}}

bool Dic::open(Parameter *in_param, FeatureTemplateSet *in_ftmpl) {//{{{
    param = in_param;
    ftmpl = in_ftmpl;

    if (darts.open(param->darts_filename.c_str()) == -1) {
        cerr << ";; Cannot open: " << param->darts_filename << endl;
        return false;
    }else{
        //cerr << ";; " << param->darts_filename  << endl;
    }
        
    MMAP_OPEN(char, dmmap, param->dic_filename.c_str());
    const char *ptr = dmmap->begin();
    token_head = reinterpret_cast<const Token *>(ptr);
        
    posid2pos.read_pos_list(param->pos_filename);
    sposid2spos.read_pos_list(param->spos_filename);
    formid2form.read_pos_list(param->form_filename);
    formtypeid2formtype.read_pos_list(param->form_type_filename);
    baseid2base.read_pos_list(param->base_filename);
    repid2rep.read_pos_list(param->rep_filename);
    readingid2reading.read_pos_list(param->reading_filename);
    imisid2imis.read_pos_list(param->imis_filename);
        
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
}//}}}

// 繰り返し系のオノマトペを自動生成する
Node* Dic::recognize_onomatopoeia(const char* start_str, size_t specified_length) {//{{{
    int code, next_code;
    Node *result_node = NULL;
        
    U8string key(start_str); // オノマトペかどうかを判定するキー
    size_t key_length = key.char_size(); /* キーの文字数を数えておく */
    std::string current_char = key[0]; // １文字目
    code = key.char_type_at(0); // 1文字目のタイプ

    /* 通常の平仮名、片仮名以外から始まるものは不可 */
    if (code != TYPE_HIRAGANA && code != TYPE_KATAKANA) return nullptr;
    // 小文字で始まる場合は終了
    if (key.is_lower(0)) return nullptr;
        
    /* 反復型オノマトペ */
    for (size_t len = 2; len < 5; len++) {// 反復の長さ
        if (key_length < len * 2) break;
        std::string current_char = key[len-1];
        /* 途中で文字種が変わるものは不可 */
        next_code = key.char_type_at(len-1);
        if (key.is_choon(len-1)) next_code = code; /* 長音は直前の文字種として扱う */
        if (code != next_code) break; // カタカナかつ記号，みたいなor を考慮していない
        code = next_code;
            
        /* 反復があるか判定 */
        if (key.char_substr(0,len) != key.char_substr(len,len)) continue;
        /* ただし3文字が同じものは不可 */
        if (key[0] == key[1] && key[1] == key[2]) continue;
        
        // specified_length を指定している場合は長さが異なる場合スキップ
        if (specified_length > 0 && (len * 2 * BYTES4CHAR) != specified_length) continue;

        Node *new_node = new Node;
        new_node->length = key.in_byte_index(len*2); //ここはバイト長
        new_node->surface = start_str;
             
        new_node->string = new std::string(new_node->surface, new_node->length);
        new_node->original_surface = new std::string(new_node->surface, new_node->length);
        new_node->string_for_print = new std::string(new_node->surface, new_node->length);
            
        new_node->posid = posid2pos.get_id(DEF_ONOMATOPOEIA_HINSI);//副詞
        new_node->pos = posid2pos.get_pos(new_node->posid);
        new_node->sposid = sposid2spos.get_id(DEF_ONOMATOPOEIA_BUNRUI);//*
        new_node->spos = sposid2spos.get_pos(new_node->sposid);
        new_node->formid = formid2form.get_id("*");//*
        new_node->form = formid2form.get_pos(new_node->formid);
        new_node->formtypeid = formtypeid2formtype.get_id("*");//*
        new_node->form_type = formtypeid2formtype.get_pos(new_node->formtypeid );//*
        new_node->baseid = baseid2base.get_id(key.char_substr(0,len*2));//
        new_node->base = baseid2base.get_pos(new_node->baseid);//
        new_node->repid = repid2rep.get_id("*");
        new_node->representation = repid2rep.get_pos(new_node->repid);
        new_node->imisid = imisid2imis.get_id(DEF_ONOMATOPOEIA_IMIS);//
        new_node->semantic_feature = imisid2imis.get_pos(new_node->imisid);
        new_node->readingid = readingid2reading.get_id("*");
        new_node->reading = readingid2reading.get_pos(new_node->readingid );
            
        new_node->char_num = utf8_chars((unsigned char *)start_str, new_node->length);
        new_node->stat = MORPH_NORMAL_NODE; 
        new_node->char_type = check_utf8_char_type((unsigned char *)start_str);
        new_node->char_family = check_char_family(new_node->char_type);
             
        char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
        new_node->end_char_family = check_char_family((unsigned char *)end_char);
        new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
            
        FeatureSet *f = new FeatureSet(ftmpl);
        f->extract_unigram_feature(new_node);
        new_node->wcost = f->calc_inner_product_with_weight();
        if(param->debug){
            new_node->debug_info["unigram_feature"] = f->str();
        }
        new_node->feature = f;

        new_node->bnext = nullptr;
        result_node = new_node;
        
//        // 以下は素性に置き換える
//        // /* weightの設定 */
//        m_buffer[m_buffer_num].weight = REPETITION_COST * len;
//        /* 拗音を含む場合 */
//        for (i = CONTRACTED_LOWERCASE_S; i < CONTRACTED_LOWERCASE_E; i++) {
//            if (strstr(m_buffer[m_buffer_num].midasi, lowercase[i])) break;
//        }
//        if (i < CONTRACTED_LOWERCASE_E) {
//            if (len == 2) continue; /* 1音の繰り返しは禁止 */		
//            /* 1文字分マイナス+ボーナス */
//            m_buffer[m_buffer_num].weight -= REPETITION_COST + CONTRACTED_BONUS;
//        }
//        /* 濁音・半濁音を含む場合 */
//        for (i = 0; *dakuon[i]; i++) {
//            if (strstr(m_buffer[m_buffer_num].midasi, dakuon[i])) break;
//        }
//        if (*dakuon[i]) {
//            m_buffer[m_buffer_num].weight -= DAKUON_BONUS; /* ボーナス */
//            /* 先頭が濁音の場合はさらにボーナス */
//            if (!strncmp(m_buffer[m_buffer_num].midasi, dakuon[i], BYTES4CHAR)) 
//                m_buffer[m_buffer_num].weight -= DAKUON_BONUS;
//        }
//        /* カタカナである場合 */
//        if (code == KATAKANA) 
//            m_buffer[m_buffer_num].weight -= KATAKANA_BONUS;
        break; /* 最初にマッチしたもののみ採用 */
    }
    return result_node;
}//}}}

// lookup_lattice の wrapper
Node *Dic::lookup_lattice(std::vector<CharLattice::da_result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length = 0, std::string *specified_pos = nullptr) {//{{{
    if (specified_pos)
        return lookup_lattice( da_search_result,start_str, specified_length, posid2pos.get_id(*specified_pos));
    else
        return lookup_lattice( da_search_result,start_str, specified_length, MORPH_DUMMY_POS);
}//}}}

// DA から検索した結果を Node に変換する 
Node *Dic::lookup_lattice(std::vector<CharLattice::da_result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length, unsigned short specified_posid) {//{{{
    Node *result_node = NULL;
        
    std::vector<CharLattice::da_result_pair_type> &result_pair = da_search_result; 
        
    if (result_pair.size() == 0)
        return result_node;
    size_t num = result_pair.size();
        
    for (size_t i = 0; i < num; i++) { 
        if (specified_length && specified_length != get_length(result_pair[i]))
            continue;
        size_t size = token_size(result_pair[i]);
        const Token *token = get_token(result_pair[i]);
                
        for (size_t j = 0; j < size; j++) { // same key but different value (pos)
            if (specified_posid != MORPH_DUMMY_POS && specified_posid != (token + j)->posid)
                continue;
                    
                    
            Node *new_node = new Node;
            read_node_info(*(token + j), &new_node);
            
            // 基本情報
            new_node->token = (Token *)(token + j);
            new_node->surface = start_str;
            new_node->length = get_length(result_pair[i]); 
            new_node->char_num = utf8_chars((unsigned char *)start_str, new_node->length);

            // 文字種関連
            new_node->char_type = check_utf8_char_type((unsigned char *)start_str);
            new_node->char_family = check_char_family(new_node->char_type);
            new_node->suuji = is_suuji((unsigned char *)start_str);
            char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
            new_node->end_char_family = check_char_family((unsigned char *)end_char);
            new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
                 
            // 末尾から二文字目を取り出す
            char *lb2_char = nullptr;
            if(new_node->char_num > 2)
                lb2_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 2);

            // ここで，NODE_PROLONG_DEL_LAST でなく，NODE_PROLONG_DEL なカタカナ語を削除する
            if( ((get_stat(result_pair[i]) & OPT_PROLONG_DEL_LAST) || (get_stat(result_pair[i]) & OPT_PROLONG_DEL)) && // 長音を付加している
                lb2_char && check_utf8_char_type((unsigned char *)start_str) == TYPE_KATAKANA && check_utf8_char_type((unsigned char *)lb2_char) == TYPE_KATAKANA){ // カタカナ語である

                // このうち，以下の条件を満たすもの以外のノードは廃棄する
                if(!((!(get_stat(result_pair[i]) & OPT_PROLONG_DEL)) && // 途中で長音を付加していない
                    new_node->posid == posid2pos.get_id("名詞"))) // 名詞である
                    continue;
            }
                
            new_node->original_surface = new std::string(start_str, new_node->length);
            //std::cout << *new_node->original_surface << "_" << *new_node->pos << std::endl;
            new_node->string_for_print = new std::string(start_str, new_node->length);
            if (new_node->lcAttr == 1) { // Wikipedia
                new_node->string = new std::string(UNK_WIKIPEDIA);
                new_node->stat = MORPH_UNK_NODE;
            } else {
                new_node->string = new std::string(*new_node->string_for_print);
                if(new_node->semantic_feature->find("濁音化",0) != std::string::npos){// TODO:意味情報を文字列扱いしない
                    new_node->stat = MORPH_DEVOICE_NODE;
                } else {
                    new_node->stat = MORPH_NORMAL_NODE;
                }
            }
                
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_unigram_feature(new_node);
            new_node->wcost = f->calc_inner_product_with_weight();
            if(param->debug){
                new_node->debug_info["unigram_feature"] = f->str();
            }
            new_node->feature = f;
                
            new_node->bnext = result_node;
            result_node = new_node;
        }
    }
    return result_node;
}//}}}

// 品詞情報を詳細に指定し, 辞書引き
Node *Dic::lookup_lattice_specified(std::vector<CharLattice::da_result_pair_type> &da_search_result, const char *start_str, unsigned int specified_length, const std::vector<std::string>& specified) {//{{{
    Node *result_node = NULL;
    
    // TODO: specified を生の vector からもう少し意味のあるものに変える
    // surf_read_base_pos_spos_type_form
    auto specified_readingid  = readingid2reading.get_id(specified[0]);
    auto specified_baseid     = baseid2base.get_id(specified[1]);
    auto specified_posid      = posid2pos.get_id(specified[2]);
    auto specified_sposid     = sposid2spos.get_id(specified[3]);
    auto specified_formtypeid = formtypeid2formtype.get_id(specified[4]);
    auto specified_formid     = formid2form.get_id(specified[5]);

    std::vector<CharLattice::da_result_pair_type> &result_pair = da_search_result; 

        
    if (result_pair.size() == 0)
        return result_node;
    size_t num = result_pair.size();
        
    for (size_t i = 0; i < num; i++) { 

        //const Token *token = get_token(result_pair[i]);// deb
        //std::cerr << start_str << "_" << *baseid2base.get_pos(token->base_id) << "_" << specified_length << "-" << result_pair[i].length << std::endl;
        if (specified_length && specified_length != get_length(result_pair[i]))
            continue;
        size_t size = token_size(result_pair[i]);
        const Token *token = get_token(result_pair[i]);

        bool modified_word = false; // 長音が挿入されていることを調べる方法が無い
                
        // std::cerr << start_str << "_" << *baseid2base.get_pos(token->base_id) << "_" << specified_length << "-" << result_pair[i].length << ":" << modified_word << std::endl;

        for (size_t j = 0; j < size; j++) { // １つでも異なればskip
            if ((!modified_word && specified[0].size()>0 && specified_readingid != (token + j)->reading_id) ||
                (!modified_word && specified[1].size()>0 && specified_baseid != (token + j)->base_id) ||
                (specified[2].size()>0 && specified_posid != (token + j)->posid) ||
                (specified[3].size()>0 && specified_sposid != (token + j)->spos_id) ||
                (specified[4].size()>0 && specified_formtypeid != (token + j)->form_type_id) ||
                (specified[5].size()>0 && specified_formid != (token + j)->form_id) )
                continue;
                
            Node *new_node = new Node;
            read_node_info(*(token + j), &new_node);
            new_node->token = (Token *)(token + j);
                
            new_node->length = get_length(result_pair[i]); 
            new_node->surface = start_str;
                
            new_node->char_num = utf8_chars((unsigned char *)start_str, new_node->length);
            new_node->original_surface = new std::string(start_str, new_node->length);
            //std::cout << *new_node->original_surface << "_" << *new_node->pos << std::endl;
            new_node->string_for_print = new std::string(start_str, new_node->length);
            if (new_node->lcAttr == 1) { // Wikipedia
                new_node->string = new std::string(UNK_WIKIPEDIA);
                new_node->stat = MORPH_UNK_NODE;
            } else {
                new_node->string = new std::string(*new_node->string_for_print);
                if(new_node->semantic_feature->find("濁音化",0) != std::string::npos){// TODO:意味情報を文字列扱いしない
                    new_node->stat = MORPH_DEVOICE_NODE;
                }else{
                    new_node->stat = MORPH_NORMAL_NODE;
                }
            }
            new_node->char_type = check_utf8_char_type((unsigned char *)start_str);
            new_node->char_family = check_char_family(new_node->char_type);
            new_node->suuji = is_suuji((unsigned char *)start_str);
            char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
            new_node->end_char_family = check_char_family((unsigned char *)end_char);
            new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
                
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_unigram_feature(new_node);
            new_node->wcost = f->calc_inner_product_with_weight();
            if(param->debug){
                new_node->debug_info["unigram_feature"] = f->str();
            }
            new_node->feature = f;
                
            new_node->bnext = result_node;
            result_node = new_node;
        }
    }
    return result_node;
}//}}}

// TODO: make_**_pseudo_node が多すぎるので，減らすか，クラス化する
// make_**pseudo_node が必要な場合はおもに３パターン
// 動的な数詞, アルファベット処理
// 文字幅の範囲を指定した未定義語処理
// 品詞などを全て与えられた，コーパス上の辞書にない単語の処理
//
// TODO: list で specified_pos な奴を廃棄 > 直接 specified_pseudo_node を呼ぶようにする
// TODO: dic_check をフラグ化 & default にする
// TODO: 名前をわかりやすくする

// MEMO: 
// make_specified_pseudo_node_by_dic_check > make_unk_pseudo_node_list_some_pos_by_dic_check > make_unk_pseudo_node 
//                                         > make_unk_pseudo_node_list_some_pos > make_unk_pseudo_node 

// make an unknown word node 
// 未定義語のノードを生成(ある品詞の候補についてのノード or 未定のままのノード(こちらはどういうタイミングで呼び出されるのか？) )
Node *Dic::make_unk_pseudo_node(const char *start_str, int byte_len, unsigned short specified_posid) {//{{{
    Node *new_node = new Node; 
    new_node->surface = start_str;
    new_node->length = byte_len;
    new_node->char_type = check_utf8_char_type((unsigned char *)new_node->surface);
    new_node->char_family = check_char_family(new_node->char_type);

    // 整理:
    // 品詞が指定されていないか，名詞で，文字が数詞なら＝＞数詞
    // 品詞が指定なしの場合, 表層(string)を未定義語に
    // 指定がある場合, 表層はそのまま

    // 品詞に関係なく共通の処理
    new_node->char_num = utf8_chars((unsigned char *)(new_node->surface), new_node->length);
    char *end_char = (char *)get_specified_char_pointer((unsigned char *)start_str, new_node->length, new_node->char_num - 1);
    new_node->end_char_family = check_char_family((unsigned char *)end_char);
    new_node->end_string = new std::string(end_char, utf8_bytes((unsigned char *)end_char));
    new_node->stat = MORPH_UNK_NODE;
    
    new_node->sposid = sposid2spos.get_id(UNK_POS);
    new_node->spos = sposid2spos.get_pos(new_node->sposid);
    new_node->readingid = readingid2reading.get_id(UNK_POS);
    new_node->reading = readingid2reading.get_pos(new_node->readingid);
    new_node->formid = formid2form.get_id(UNK_POS);
    new_node->form = formid2form.get_pos(new_node->formid);
    new_node->formtypeid = formtypeid2formtype.get_id(UNK_POS);
    new_node->form_type = formtypeid2formtype.get_pos(new_node->formtypeid);
    new_node->repid = repid2rep.get_id(UNK_POS);
    new_node->representation = repid2rep.get_pos(new_node->repid);
    new_node->imisid = imisid2imis.get_id("NIL");
    new_node->semantic_feature = imisid2imis.get_pos(new_node->imisid);
    
    new_node->original_surface = new std::string(new_node->surface, new_node->length);
    new_node->string_for_print = new std::string(new_node->surface, new_node->length);
    

    if((new_node->char_type == TYPE_FIGURE||new_node->char_type == TYPE_KANJI_FIGURE) && (specified_posid == posid2pos.get_id("名詞")|| specified_posid == MORPH_DUMMY_POS)){
        //数詞の場合
        new_node->string = new std::string("<数詞>");
        new_node->baseid = baseid2base.get_id("<数詞>");
        new_node->base = baseid2base.get_pos(new_node->baseid);
        new_node->string = new std::string("<数詞>");
        new_node->posid = posid2pos.get_id("名詞");
        new_node->pos = posid2pos.get_pos(new_node->posid);
        new_node->sposid = sposid2spos.get_id("数詞");
        new_node->spos = sposid2spos.get_pos(new_node->sposid );
        new_node->formid = formid2form.get_id("*");
        new_node->form = formid2form.get_pos(new_node->formid);
        new_node->formtypeid = formtypeid2formtype.get_id("*");
        new_node->form_type = formtypeid2formtype.get_pos(new_node->formtypeid );
        new_node->base = new std::string("<数詞>");
    }else{
        if (new_node->char_type == TYPE_KANJI){
            new_node->string = new std::string("未定義漢語");
            new_node->baseid = baseid2base.get_id("未定義漢語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } else if (new_node->char_type == TYPE_KATAKANA){
            new_node->string = new std::string("未定義カタカナ語");
            new_node->baseid = baseid2base.get_id("未定義カタカナ語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } else if (new_node->char_type == TYPE_HIRAGANA){
            new_node->string = new std::string("未定義ひらがな語");
            new_node->baseid = baseid2base.get_id("未定義ひらがな語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } else {//アルファベット,記号など？
            new_node->string = new std::string(new_node->surface, new_node->length);
            new_node->baseid = baseid2base.get_id("未定義アルファベット語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } //漢字かな交じりは現状では扱っていない
      
        // pos が指定されている場合とそうでない場合で分岐する
        // 品詞などを埋める
        if (specified_posid == MORPH_DUMMY_POS) {
            new_node->posid  = specified_posid;
            new_node->pos = posid2pos.get_pos(new_node->posid);
        } else {//品詞が指定されている場合
            new_node->posid  = specified_posid;
            new_node->pos = posid2pos.get_pos(new_node->posid);
        }
    }
        
    new_node->feature = new FeatureSet(ftmpl);
    new_node->feature->extract_unigram_feature(new_node);
    new_node->wcost = new_node->feature->calc_inner_product_with_weight();

    if(param->debug){
        new_node->debug_info["unigram_feature"] = new_node->feature->str();
    }
    return new_node;
}//}}}

// make_chartype_pseud_node
//sentence.cc:236: 
//sentence.cc:242: 
//sentence.cc:252: 
//sentence.cc:323: 
//sentence.cc:329: 
//sentence.cc:339: 
// 名前が変？指定した文字種が連続する範囲で全品詞について未定義語ノードを生成
// 辞書をr_node で受け取り、重複をチェック
Node *Dic::make_specified_pseudo_node_by_dic_check(const char *start_str, unsigned int specified_length, std::string *specified_pos, std::vector<unsigned short> *specified_unk_pos, unsigned int type_family, Node* r_node) {//{{{
    unsigned short specified_posid = MORPH_DUMMY_POS;
    if(specified_pos) specified_posid = posid2pos.get_id(*specified_pos);

    U8string ustart_str = (std::string(start_str));

    unsigned int length = strlen(start_str);
    unsigned int pos = 0, char_num = 0;
    // 文字種が連続する範囲をチェック
    for (pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(start_str + pos))) {
        unsigned int used_chars = 0;
        // exceptional figure expression of two characters
        // 数字を指定していて、かつ数字の例外に当たる場合
        if ( pos != 0  && type_family == TYPE_FAMILY_FIGURE && (used_chars = check_exceptional_chars_in_figure(start_str + pos , length - pos)) > 0 ) {
            if(used_chars == 2)
                pos += utf8_bytes((unsigned char *)(start_str + pos));
            else if(used_chars == 3){
                pos += utf8_bytes((unsigned char *)(start_str + pos));
                pos += utf8_bytes((unsigned char *)(start_str + pos));
            }
                
            char_num += used_chars;
        } // doesn't start with slash, colon, etc.
        else if (pos == 0 && compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FAMILY_PUNC_SYMBOL))
            break;
        else if (pos == 0 && ustart_str.is_choon(0) )//一文字目が伸ばし棒
            break;
        else if (( param->use_suu_rule && (pos == 0 && ustart_str.is_figure_exception(0)) ) &&  // 数が先頭に出現している
                 (ustart_str.char_size()==1 ||                                // 続く文字がない場合(数は単体で数詞として辞書に登録済み) ，もしくは
                 (ustart_str.is_suuji(1) && !ustart_str.is_suuji_digit(1)))){ // 次が桁を表す数字でない場合
            // 数十，数百への対応
            // 数の次に桁を表す文字が来ない場合は，ひとかたまりの数詞として扱わない．
            // 数キロ，等は先に上のif文でチェックしているため，ここでは考慮しない
            break;
        } else if ( param->use_suu_rule && char_num > 0 && ustart_str.is_figure_exception(char_num) && !ustart_str.is_suuji_digit(char_num-1)){ 
            // 十数人，などへの対応
            // 十や百など，桁の次に"数" が出てきた場合には，ひとかたまりの数詞として扱う
            break;
        } else if (compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), type_family)) // this is in specified family
            char_num++;
        else
            break;
    }

    if (char_num > 0 && (specified_length == 0 || specified_length == pos)) {
        if( r_node ) //辞書が渡されている場合
            return make_unk_pseudo_node_list_some_pos_by_dic_check(start_str, pos, specified_posid, specified_unk_pos, r_node);
        else
            return make_unk_pseudo_node_list_some_pos(start_str, pos, specified_posid, specified_unk_pos); 
    }
    else {
        return nullptr;
    }

}//}}}

// 辞書と重複しないものだけを作る
// 作ってr_node に足すことはしない
// 何も作られなかった場合は nullptr を返す
// 重複してるとき，delteしてないからリークしてる疑惑
Node *Dic::make_unk_pseudo_node_list_some_pos_by_dic_check(const char *start_str, int byte_len, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos, Node* r_node) {//{{{
    Node *result_node = nullptr;
    if (specified_posid == MORPH_DUMMY_POS ) {
        //result_node = r_node; // こうするべきか，しないべきか??
        for (std::vector<unsigned short>::iterator it = specified_unk_pos->begin(); it != specified_unk_pos->end(); it++) {
            Node *new_node = make_unk_pseudo_node(start_str, byte_len, *it);

            bool flag_covered = false;
            Node *tmp_node = r_node;
            if(r_node)
                while (tmp_node->bnext){
                    if(tmp_node->length == new_node->length && tmp_node->posid == new_node->posid)
                        flag_covered = true;
                    tmp_node = tmp_node->bnext;
                } //辞書チェック

            if( !flag_covered || new_node->sposid == sposid2spos.get_id("数詞") ){
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
        if(!flag_covered || new_node->sposid == sposid2spos.get_id("数詞") ){
            new_node->bnext = result_node;
            result_node = new_node;
        }
    }
    return result_node;
}//}}}

// _list では未定義語としてノードを作る際に品詞候補についてそれぞれノードを生成する
// ほぼNode 作るだけ( 品詞が指定されていれば、その品詞、UNKなら候補すべて)
Node *Dic::make_unk_pseudo_node_list_some_pos(const char *start_str, int byte_len, unsigned short specified_posid, std::vector<unsigned short> *specified_unk_pos) {//{{{
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
}//}}}

// 連続する同じ文字種の文字列を未定義語化
// make_unk_pseudo_node_list_from_previous_position 専用
// min_char_num から max_char_num までの範囲におさまるもののみ
// make unknown word nodes of some lengths
// TODO make_unk_pseudo_node_list_some_pos を長さの範囲だけ呼び出してマージするようにする
Node *Dic::make_pseudo_node_list_in_range(const char *start_str, unsigned int min_char_num, unsigned int max_char_num, unsigned short specified_posid) {//{{{
    Node *result_node = NULL;
    unsigned int length = strlen(start_str), char_num = 1;
    unsigned long code = 0; 
    unsigned long last_code = 0; 
    // 
    for (unsigned int pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(start_str + pos))) {
        code = check_utf8_char_type((unsigned char *)(start_str + pos));
            
        // 異なる文字種が連続する場合には未定義語にしない
        if (((code & (TYPE_HIRAGANA|TYPE_KANJI|TYPE_KATAKANA|TYPE_ALPH)) & (last_code & (TYPE_HIRAGANA|TYPE_KANJI|TYPE_KATAKANA|TYPE_ALPH))) == 0 && last_code!=0)
            break; 
        
        // figures and alphabets that are processed separately
        if (compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FIGURE | TYPE_KANJI_FIGURE | TYPE_ALPH))
            break;
        // stop if it's a punctuation or a space
        else if (pos > 0 && compare_char_type_in_family(check_utf8_char_type((unsigned char *)(start_str + pos)), TYPE_FAMILY_PUNC | TYPE_FAMILY_SPACE))
            break;
        else if (char_num > max_char_num) // max characters as <UNK>
            break;
        else if (char_num < min_char_num) { // skip while the length is shorter than the specified length
            last_code = code;
            char_num++;
            continue;
        }

        Node *new_node = make_unk_pseudo_node_list_some_pos(start_str, pos + utf8_bytes((unsigned char *)(start_str + pos)), specified_posid, &(param->unk_pos));
        Node *tmp_node = new_node;
        while (tmp_node->bnext)
            tmp_node = tmp_node->bnext;
        tmp_node->bnext = result_node;
        result_node = new_node;
        last_code = code;
        char_num++;
    }
    return result_node;
}//}}}


// あとでオプションに変更する
Node *Dic::make_unk_pseudo_node_gold(const char *start_str, int byte_len, std::string &specified_pos) {//{{{
    unsigned short specified_posid = posid2pos.get_id(specified_pos);

    Node *new_node = new Node;
    new_node->surface = start_str;
    new_node->length = byte_len;
    new_node->char_type = check_utf8_char_type((unsigned char *)new_node->surface);
    new_node->char_family = check_char_family(new_node->char_type);
    new_node->string = new std::string(new_node->surface, new_node->length);
    new_node->original_surface = new std::string(new_node->surface, new_node->length);
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
        new_node->baseid = baseid2base.get_id(UNK_POS); //文字種に置き換える
        new_node->readingid = readingid2reading.get_id(UNK_POS);


        // 却って悪くなるかも？05/25
        if (new_node->char_type == TYPE_KANJI){
            delete new_node->string;
            new_node->string = new std::string("未定義漢語");
            new_node->baseid = baseid2base.get_id("未定義漢語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } else if (new_node->char_type == TYPE_KATAKANA){
            delete new_node->string;
            new_node->string = new std::string("未定義カタカナ語");
            new_node->baseid = baseid2base.get_id("未定義カタカナ語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } else if (new_node->char_type == TYPE_HIRAGANA){
            delete new_node->string;
            new_node->string = new std::string("未定義ひらがな語");
            new_node->baseid = baseid2base.get_id("未定義ひらがな語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } else {//アルファベット,記号など？
            delete new_node->string;
            new_node->string = new std::string(new_node->surface, new_node->length);
            new_node->baseid = baseid2base.get_id("未定義アルファベット語");
            new_node->base = baseid2base.get_pos(new_node->baseid);
        } //漢字かな交じりは現状では扱っていない

        new_node->repid = repid2rep.get_id("*");
        new_node->imisid = imisid2imis.get_id("NIL");
    }
    new_node->pos = posid2pos.get_pos(new_node->posid);
    new_node->spos = sposid2spos.get_pos(new_node->sposid);
    new_node->form = formid2form.get_pos(new_node->formid);
    new_node->form_type = formtypeid2formtype.get_pos(new_node->formtypeid);
    new_node->base = baseid2base.get_pos(new_node->baseid);
    new_node->reading = readingid2reading.get_pos(new_node->readingid);
    
    new_node->representation = repid2rep.get_pos(new_node->repid);
    new_node->semantic_feature = imisid2imis.get_pos(new_node->imisid);

    new_node->feature = new FeatureSet(ftmpl);
    new_node->feature->extract_unigram_feature(new_node);
    new_node->wcost = new_node->feature->calc_inner_product_with_weight();
    //new_node->debug_info["unigram_feature"] = new_node->feature->str();

    return new_node;
}//}}}
// mkdarts => Dic
void inline Dic::read_node_info(const Token &token, Node **node) {//{{{
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
    (*node)->readingid = token.reading_id;
    (*node)->reading = readingid2reading.get_pos((*node)->readingid);

    (*node)->repid = token.rep_id;
    (*node)->imisid = token.imis_id;

    // 代表表記の読み込み
    (*node)->representation = repid2rep.get_pos((*node)->repid);
    //cout << (*node)->representation << endl;
    // 意味情報の読み込み
    (*node)->semantic_feature = imisid2imis.get_pos((*node)->imisid);

    //(*node)->wcost = token.wcost;
    (*node)->wcost = 0.0;
    //(*node)->token = const_cast<Token *>(&token); //保存する必要無い
}//}}}

// constant mappings
const std::unordered_map<std::string,int> Dic::pos_map({//{{{
    {"*",0}, {"特殊",1}, {"動詞",2}, {"形容詞",3}, {"判定詞",4}, {"助動詞",5}, {"名詞",6}, {"指示詞",7},
    {"副詞",8}, {"助詞",9}, {"接続詞",10}, {"連体詞",11}, {"感動詞",12}, {"接頭辞",13}, {"接尾辞",14}, {"未定義語",15}});//}}}
const std::unordered_map<std::string,int> Dic::spos_map{//{{{
    {"*",0},
    {"句点",1}, {"読点",2}, {"括弧始",3}, {"括弧終",4}, {"記号",5}, {"空白",6}, 
    {"普通名詞",1}, {"サ変名詞",2}, {"固有名詞",3}, {"地名",4}, {"人名",5}, {"組織名",6}, {"数詞",7}, {"形式名詞",8},
    {"副詞的名詞",9}, {"時相名詞",10}, {"名詞形態指示詞",1}, {"連体詞形態指示詞",2}, {"副詞形態指示詞",3}, {"格助詞",1}, {"副助詞",2}, 
    {"接続助詞",3}, {"終助詞",4}, {"名詞接頭辞",1}, {"動詞接頭辞",2}, {"イ形容詞接頭辞",3}, {"ナ形容詞接頭辞",4}, {"名詞性述語接尾辞",1}, 
    {"名詞性名詞接尾辞",2}, {"名詞性名詞助数辞",3}, {"名詞性特殊接尾辞",4}, {"形容詞性述語接尾辞",5}, {"形容詞性名詞接尾辞",6}, 
    {"動詞性接尾辞",7}, {"その他",1}, {"カタカナ",2}, {"アルファベット",3}};//}}}
const std::unordered_map<std::string,int> Dic::katuyou_type_map{//{{{
    {"*",0}, {"母音動詞",1}, {"子音動詞カ行",2}, {"子音動詞カ行促音便形",3}, {"子音動詞ガ行",4}, {"子音動詞サ行",5},
    {"子音動詞タ行",6}, {"子音動詞ナ行",7}, {"子音動詞バ行",8}, {"子音動詞マ行",9}, {"子音動詞ラ行",10},
    {"子音動詞ラ行イ形",11}, {"子音動詞ワ行",12}, {"子音動詞ワ行文語音便形",13}, {"カ変動詞",14}, {"カ変動詞来",15},
    {"サ変動詞",16}, {"ザ変動詞",17}, {"イ形容詞アウオ段",18}, {"イ形容詞イ段",19}, {"イ形容詞イ段特殊",20},
    {"ナ形容詞",21}, {"ナノ形容詞",22}, {"ナ形容詞特殊",23}, {"タル形容詞",24}, {"判定詞",25},
    {"無活用型",26}, {"助動詞ぬ型",27}, {"助動詞だろう型",28}, {"助動詞そうだ型",29},
    {"助動詞く型",30}, {"動詞性接尾辞ます型",31}, {"動詞性接尾辞うる型",32}};//}}}
const std::unordered_map<std::string,int> Dic::katuyou_form_map{{"母音動詞:語幹",1},//{{{
    {"*:*",0},
{"母音動詞:基本形",2}, {"母音動詞:未然形",3}, {"母音動詞:意志形",4}, {"母音動詞:省略意志形",5}, {"母音動詞:命令形",6}, {"母音動詞:基本条件形",7},
{"母音動詞:基本連用形",8}, {"母音動詞:タ接連用形",9}, {"母音動詞:タ形",10}, {"母音動詞:タ系推量形",11}, {"母音動詞:タ系省略推量形",12},
{"母音動詞:タ系条件形",13}, {"母音動詞:タ系連用テ形",14}, {"母音動詞:タ系連用タリ形",15}, {"母音動詞:タ系連用チャ形",16}, {"母音動詞:音便条件形",17},
{"母音動詞:文語命令形",18}, {"子音動詞カ行:語幹",1}, {"子音動詞カ行:基本形",2}, {"子音動詞カ行:未然形",3}, {"子音動詞カ行:意志形",4},
{"子音動詞カ行:省略意志形",5}, {"子音動詞カ行:命令形",6}, {"子音動詞カ行:基本条件形",7}, {"子音動詞カ行:基本連用形",8}, {"子音動詞カ行:タ接連用形",9},
{"子音動詞カ行:タ形",10}, {"子音動詞カ行:タ系推量形",11}, {"子音動詞カ行:タ系省略推量形",12}, {"子音動詞カ行:タ系条件形",13}, {"子音動詞カ行:タ系連用テ形",14}, {"子音動詞カ行:タ系連用タリ形",15}, {"子音動詞カ行:タ系連用チャ形",16}, {"子音動詞カ行:音便条件形",17}, {"子音動詞カ行促音便形:語幹",1},
{"子音動詞カ行促音便形:基本形",2}, {"子音動詞カ行促音便形:未然形",3}, {"子音動詞カ行促音便形:意志形",4}, {"子音動詞カ行促音便形:省略意志形",5}, {"子音動詞カ行促音便形:命令形",6}, {"子音動詞カ行促音便形:基本条件形",7}, {"子音動詞カ行促音便形:基本連用形",8}, {"子音動詞カ行促音便形:タ接連用形",9}, {"子音動詞カ行促音便形:タ形",10}, {"子音動詞カ行促音便形:タ系推量形",11}, {"子音動詞カ行促音便形:タ系省略推量形",12}, {"子音動詞カ行促音便形:タ系条件形",13}, {"子音動詞カ行促音便形:タ系連用テ形",14},
{"子音動詞カ行促音便形:タ系連用タリ形",15}, {"子音動詞カ行促音便形:タ系連用チャ形",16}, {"子音動詞カ行促音便形:音便条件形",17}, {"子音動詞ガ行:語幹",1}, {"子音動詞ガ行:基本形",2},
{"子音動詞ガ行:未然形",3}, {"子音動詞ガ行:意志形",4}, {"子音動詞ガ行:省略意志形",5}, {"子音動詞ガ行:命令形",6}, {"子音動詞ガ行:基本条件形",7},
{"子音動詞ガ行:基本連用形",8}, {"子音動詞ガ行:タ接連用形",9}, {"子音動詞ガ行:タ形",10}, {"子音動詞ガ行:タ系推量形",11}, {"子音動詞ガ行:タ系省略推量形",12}, {"子音動詞ガ行:タ系条件形",13}, {"子音動詞ガ行:タ系連用テ形",14}, {"子音動詞ガ行:タ系連用タリ形",15}, {"子音動詞ガ行:タ系連用チャ形",16},
{"子音動詞ガ行:音便条件形",17}, {"子音動詞サ行:語幹",1}, {"子音動詞サ行:基本形",2}, {"子音動詞サ行:未然形",3}, {"子音動詞サ行:意志形",4},
{"子音動詞サ行:省略意志形",5}, {"子音動詞サ行:命令形",6}, {"子音動詞サ行:基本条件形",7}, {"子音動詞サ行:基本連用形",8}, {"子音動詞サ行:タ接連用形",9},
{"子音動詞サ行:タ形",10}, {"子音動詞サ行:タ系推量形",11}, {"子音動詞サ行:タ系省略推量形",12}, {"子音動詞サ行:タ系条件形",13}, {"子音動詞サ行:タ系連用テ形",14}, {"子音動詞サ行:タ系連用タリ形",15}, {"子音動詞サ行:タ系連用チャ形",16}, {"子音動詞サ行:音便条件形",17}, {"子音動詞タ行:語幹",1},
{"子音動詞タ行:基本形",2}, {"子音動詞タ行:未然形",3}, {"子音動詞タ行:意志形",4}, {"子音動詞タ行:省略意志形",5}, {"子音動詞タ行:命令形",6},
{"子音動詞タ行:基本条件形",7}, {"子音動詞タ行:基本連用形",8}, {"子音動詞タ行:タ接連用形",9}, {"子音動詞タ行:タ形",10}, {"子音動詞タ行:タ系推量形",11},
{"子音動詞タ行:タ系省略推量形",12}, {"子音動詞タ行:タ系条件形",13}, {"子音動詞タ行:タ系連用テ形",14}, {"子音動詞タ行:タ系連用タリ形",15}, {"子音動詞タ行:タ系連用チャ形",16}, {"子音動詞タ行:音便条件形",17}, {"子音動詞ナ行:語幹",1}, {"子音動詞ナ行:基本形",2}, {"子音動詞ナ行:未然形",3},
{"子音動詞ナ行:意志形",4}, {"子音動詞ナ行:省略意志形",5}, {"子音動詞ナ行:命令形",6}, {"子音動詞ナ行:基本条件形",7}, {"子音動詞ナ行:基本連用形",8},
{"子音動詞ナ行:タ接連用形",9}, {"子音動詞ナ行:タ形",10}, {"子音動詞ナ行:タ系推量形",11}, {"子音動詞ナ行:タ系省略推量形",12}, {"子音動詞ナ行:タ系条件形",13}, {"子音動詞ナ行:タ系連用テ形",14}, {"子音動詞ナ行:タ系連用タリ形",15}, {"子音動詞ナ行:タ系連用チャ形",16}, {"子音動詞ナ行:音便条件形",17},
{"子音動詞バ行:語幹",1}, {"子音動詞バ行:基本形",2}, {"子音動詞バ行:未然形",3}, {"子音動詞バ行:意志形",4}, {"子音動詞バ行:省略意志形",5},
{"子音動詞バ行:命令形",6}, {"子音動詞バ行:基本条件形",7}, {"子音動詞バ行:基本連用形",8}, {"子音動詞バ行:タ接連用形",9}, {"子音動詞バ行:タ形",10},
{"子音動詞バ行:タ系推量形",11}, {"子音動詞バ行:タ系省略推量形",12}, {"子音動詞バ行:タ系条件形",13}, {"子音動詞バ行:タ系連用テ形",14}, {"子音動詞バ行:タ系連用タリ形",15}, {"子音動詞バ行:タ系連用チャ形",16}, {"子音動詞バ行:音便条件形",17}, {"子音動詞マ行:語幹",1}, {"子音動詞マ行:基本形",2},
{"子音動詞マ行:未然形",3}, {"子音動詞マ行:意志形",4}, {"子音動詞マ行:省略意志形",5}, {"子音動詞マ行:命令形",6}, {"子音動詞マ行:基本条件形",7},
{"子音動詞マ行:基本連用形",8}, {"子音動詞マ行:タ接連用形",9}, {"子音動詞マ行:タ形",10}, {"子音動詞マ行:タ系推量形",11}, {"子音動詞マ行:タ系省略推量形",12}, {"子音動詞マ行:タ系条件形",13}, {"子音動詞マ行:タ系連用テ形",14}, {"子音動詞マ行:タ系連用タリ形",15}, {"子音動詞マ行:タ系連用チャ形",16},
{"子音動詞マ行:音便条件形",17}, {"子音動詞ラ行:語幹",1}, {"子音動詞ラ行:基本形",2}, {"子音動詞ラ行:未然形",3}, {"子音動詞ラ行:意志形",4},
{"子音動詞ラ行:省略意志形",5}, {"子音動詞ラ行:命令形",6}, {"子音動詞ラ行:基本条件形",7}, {"子音動詞ラ行:基本連用形",8}, {"子音動詞ラ行:タ接連用形",9},
{"子音動詞ラ行:タ形",10}, {"子音動詞ラ行:タ系推量形",11}, {"子音動詞ラ行:タ系省略推量形",12}, {"子音動詞ラ行:タ系条件形",13}, {"子音動詞ラ行:タ系連用テ形",14}, {"子音動詞ラ行:タ系連用タリ形",15}, {"子音動詞ラ行:タ系連用チャ形",16}, {"子音動詞ラ行:音便条件形",17}, {"子音動詞ラ行イ形:語幹",1},
{"子音動詞ラ行イ形:基本形",2}, {"子音動詞ラ行イ形:未然形",3}, {"子音動詞ラ行イ形:意志形",4}, {"子音動詞ラ行イ形:省略意志形",5}, {"子音動詞ラ行イ形:命令形",6}, {"子音動詞ラ行イ形:基本条件形",7}, {"子音動詞ラ行イ形:基本連用形",8}, {"子音動詞ラ行イ形:タ接連用形",9}, {"子音動詞ラ行イ形:タ形",10},
{"子音動詞ラ行イ形:タ系推量形",11}, {"子音動詞ラ行イ形:タ系省略推量形",12}, {"子音動詞ラ行イ形:タ系条件形",13}, {"子音動詞ラ行イ形:タ系連用テ形",14}, {"子音動詞ラ行イ形:タ系連用タリ形",15}, {"子音動詞ラ行イ形:タ系連用チャ形",16}, {"子音動詞ラ行イ形:音便条件形",17}, {"子音動詞ワ行:語幹",1}, {"子音動詞ワ行:基本形",2}, {"子音動詞ワ行:未然形",3}, {"子音動詞ワ行:意志形",4}, {"子音動詞ワ行:省略意志形",5}, {"子音動詞ワ行:命令形",6},
{"子音動詞ワ行:基本条件形",7}, {"子音動詞ワ行:基本連用形",8}, {"子音動詞ワ行:タ接連用形",9}, {"子音動詞ワ行:タ形",10}, {"子音動詞ワ行:タ系推量形",11},
{"子音動詞ワ行:タ系省略推量形",12}, {"子音動詞ワ行:タ系条件形",13}, {"子音動詞ワ行:タ系連用テ形",14}, {"子音動詞ワ行:タ系連用タリ形",15}, {"子音動詞ワ行:タ系連用チャ形",16}, {"子音動詞ワ行文語音便形:語幹",1}, {"子音動詞ワ行文語音便形:基本形",2}, {"子音動詞ワ行文語音便形:未然形",3}, {"子音動詞ワ行文語音便形:意志形",4}, {"子音動詞ワ行文語音便形:省略意志形",5}, {"子音動詞ワ行文語音便形:命令形",6}, {"子音動詞ワ行文語音便形:基本条件形",7}, {"子音動詞ワ行文語音便形:基本連用形",8}, {"子音動詞ワ行文語音便形:タ接連用形",9}, {"子音動詞ワ行文語音便形:タ形",10}, {"子音動詞ワ行文語音便形:タ系推量形",11}, {"子音動詞ワ行文語音便形:タ系省略推量形",12}, {"子音動詞ワ行文語音便形:タ系条件形",13}, {"子音動詞ワ行文語音便形:タ系連用テ形",14}, {"子音動詞ワ行文語音便形:タ系連用タリ形",15}, {"子音動詞ワ行文語音便形:タ系連用チャ形",16},
{"カ変動詞:語幹",1}, {"カ変動詞:基本形",2}, {"カ変動詞:未然形",3}, {"カ変動詞:意志形",4}, {"カ変動詞:省略意志形",5},
{"カ変動詞:命令形",6}, {"カ変動詞:基本条件形",7}, {"カ変動詞:基本連用形",8}, {"カ変動詞:タ接連用形",9}, {"カ変動詞:タ形",10},
{"カ変動詞:タ系推量形",11}, {"カ変動詞:タ系省略推量形",12}, {"カ変動詞:タ系条件形",13}, {"カ変動詞:タ系連用テ形",14}, {"カ変動詞:タ系連用タリ形",15},
{"カ変動詞:タ系連用チャ形",16}, {"カ変動詞:音便条件形",17}, {"カ変動詞来:語幹",1}, {"カ変動詞来:基本形",2}, {"カ変動詞来:未然形",3},
{"カ変動詞来:意志形",4}, {"カ変動詞来:省略意志形",5}, {"カ変動詞来:命令形",6}, {"カ変動詞来:基本条件形",7}, {"カ変動詞来:基本連用形",8},
{"カ変動詞来:タ接連用形",9}, {"カ変動詞来:タ形",10}, {"カ変動詞来:タ系推量形",11}, {"カ変動詞来:タ系省略推量形",12}, {"カ変動詞来:タ系条件形",13},
{"カ変動詞来:タ系連用テ形",14}, {"カ変動詞来:タ系連用タリ形",15}, {"カ変動詞来:タ系連用チャ形",16}, {"カ変動詞来:音便条件形",17}, {"サ変動詞:語幹",1},
{"サ変動詞:基本形",2}, {"サ変動詞:未然形",3}, {"サ変動詞:意志形",4}, {"サ変動詞:省略意志形",5}, {"サ変動詞:命令形",6},
{"サ変動詞:基本条件形",7}, {"サ変動詞:基本連用形",8}, {"サ変動詞:タ接連用形",9}, {"サ変動詞:タ形",10}, {"サ変動詞:タ系推量形",11},
{"サ変動詞:タ系省略推量形",12}, {"サ変動詞:タ系条件形",13}, {"サ変動詞:タ系連用テ形",14}, {"サ変動詞:タ系連用タリ形",15}, {"サ変動詞:タ系連用チャ形",16}, {"サ変動詞:音便条件形",17}, {"サ変動詞:文語基本形",18}, {"サ変動詞:文語未然形",19}, {"サ変動詞:文語命令形",20},
{"ザ変動詞:語幹",1}, {"ザ変動詞:基本形",2}, {"ザ変動詞:未然形",3}, {"ザ変動詞:意志形",4}, {"ザ変動詞:省略意志形",5},
{"ザ変動詞:命令形",6}, {"ザ変動詞:基本条件形",7}, {"ザ変動詞:基本連用形",8}, {"ザ変動詞:タ接連用形",9}, {"ザ変動詞:タ形",10},
{"ザ変動詞:タ系推量形",11}, {"ザ変動詞:タ系省略推量形",12}, {"ザ変動詞:タ系条件形",13}, {"ザ変動詞:タ系連用テ形",14}, {"ザ変動詞:タ系連用タリ形",15},
{"ザ変動詞:タ系連用チャ形",16}, {"ザ変動詞:音便条件形",17}, {"ザ変動詞:文語基本形",18}, {"ザ変動詞:文語未然形",19}, {"ザ変動詞:文語命令形",20},
{"イ形容詞アウオ段:語幹",1}, {"イ形容詞アウオ段:基本形",2}, {"イ形容詞アウオ段:命令形",3}, {"イ形容詞アウオ段:基本推量形",4}, {"イ形容詞アウオ段:基本省略推量形",5}, {"イ形容詞アウオ段:基本条件形",6}, {"イ形容詞アウオ段:基本連用形",7}, {"イ形容詞アウオ段:タ形",8}, {"イ形容詞アウオ段:タ系推量形",9},
{"イ形容詞アウオ段:タ系省略推量形",10}, {"イ形容詞アウオ段:タ系条件形",11}, {"イ形容詞アウオ段:タ系連用テ形",12}, {"イ形容詞アウオ段:タ系連用タリ形",13}, {"イ形容詞アウオ段:タ系連用チャ形",14},
{"イ形容詞アウオ段:タ系連用チャ形２",15}, {"イ形容詞アウオ段:音便条件形",16}, {"イ形容詞アウオ段:音便条件形２",17}, {"イ形容詞アウオ段:文語基本形",18}, {"イ形容詞アウオ段:文語未然形",19},
{"イ形容詞アウオ段:文語連用形",20}, {"イ形容詞アウオ段:文語連体形",21}, {"イ形容詞アウオ段:文語命令形",22}, {"イ形容詞アウオ段:エ基本形",23}, {"イ形容詞イ段:語幹",1}, {"イ形容詞イ段:基本形",2}, {"イ形容詞イ段:命令形",3}, {"イ形容詞イ段:基本推量形",4}, {"イ形容詞イ段:基本省略推量形",5},
{"イ形容詞イ段:基本条件形",6}, {"イ形容詞イ段:基本連用形",7}, {"イ形容詞イ段:タ形",8}, {"イ形容詞イ段:タ系推量形",9}, {"イ形容詞イ段:タ系省略推量形",10}, {"イ形容詞イ段:タ系条件形",11}, {"イ形容詞イ段:タ系連用テ形",12}, {"イ形容詞イ段:タ系連用タリ形",13}, {"イ形容詞イ段:タ系連用チャ形",14},
{"イ形容詞イ段:タ系連用チャ形２",15}, {"イ形容詞イ段:音便条件形",16}, {"イ形容詞イ段:音便条件形２",17}, {"イ形容詞イ段:文語基本形",18}, {"イ形容詞イ段:文語未然形",19}, {"イ形容詞イ段:文語連用形",20}, {"イ形容詞イ段:文語連体形",21}, {"イ形容詞イ段:文語命令形",22}, {"イ形容詞イ段:エ基本形",23},
{"イ形容詞イ段特殊:語幹",1}, {"イ形容詞イ段特殊:基本形",2}, {"イ形容詞イ段特殊:命令形",3}, {"イ形容詞イ段特殊:基本推量形",4}, {"イ形容詞イ段特殊:基本省略推量形",5}, {"イ形容詞イ段特殊:基本条件形",6}, {"イ形容詞イ段特殊:基本連用形",7}, {"イ形容詞イ段特殊:タ形",8}, {"イ形容詞イ段特殊:タ系推量形",9},
{"イ形容詞イ段特殊:タ系省略推量形",10}, {"イ形容詞イ段特殊:タ系条件形",11}, {"イ形容詞イ段特殊:タ系連用テ形",12}, {"イ形容詞イ段特殊:タ系連用タリ形",13}, {"イ形容詞イ段特殊:タ系連用チャ形",14},
{"イ形容詞イ段特殊:タ系連用チャ形２",15}, {"イ形容詞イ段特殊:音便条件形",16}, {"イ形容詞イ段特殊:音便条件形２",17}, {"イ形容詞イ段特殊:文語基本形",18}, {"イ形容詞イ段特殊:文語未然形",19},
{"イ形容詞イ段特殊:文語連用形",20}, {"イ形容詞イ段特殊:文語連体形",21}, {"イ形容詞イ段特殊:文語命令形",22}, {"イ形容詞イ段特殊:エ基本形",23}, {"ナ形容詞:語幹",1}, {"ナ形容詞:基本形",2}, {"ナ形容詞:ダ列基本連体形",3}, {"ナ形容詞:ダ列基本推量形",4}, {"ナ形容詞:ダ列基本省略推量形",5},
{"ナ形容詞:ダ列基本条件形",6}, {"ナ形容詞:ダ列基本連用形",7}, {"ナ形容詞:ダ列タ形",8}, {"ナ形容詞:ダ列タ系推量形",9}, {"ナ形容詞:ダ列タ系省略推量形",10}, {"ナ形容詞:ダ列タ系条件形",11}, {"ナ形容詞:ダ列タ系連用テ形",12}, {"ナ形容詞:ダ列タ系連用タリ形",13}, {"ナ形容詞:ダ列タ系連用ジャ形",14},
{"ナ形容詞:ダ列文語連体形",15}, {"ナ形容詞:ダ列文語条件形",16}, {"ナ形容詞:デアル列基本形",17}, {"ナ形容詞:デアル列命令形",18}, {"ナ形容詞:デアル列基本推量形",19}, {"ナ形容詞:デアル列基本省略推量形",20}, {"ナ形容詞:デアル列基本条件形",21}, {"ナ形容詞:デアル列基本連用形",22}, {"ナ形容詞:デアル列タ形",23}, {"ナ形容詞:デアル列タ系推量形",24}, {"ナ形容詞:デアル列タ系省略推量形",25}, {"ナ形容詞:デアル列タ系条件形",26}, {"ナ形容詞:デアル列タ系連用テ形",27}, {"ナ形容詞:デアル列タ系連用タリ形",28}, {"ナ形容詞:デス列基本形",29}, {"ナ形容詞:デス列基本推量形",30}, {"ナ形容詞:デス列基本省略推量形",31},
{"ナ形容詞:デス列タ形",32}, {"ナ形容詞:デス列タ系推量形",33}, {"ナ形容詞:デス列タ系省略推量形",34}, {"ナ形容詞:デス列タ系条件形",35}, {"ナ形容詞:デス列タ系連用テ形",36}, {"ナ形容詞:デス列タ系連用タリ形",37}, {"ナ形容詞:ヤ列基本形",38}, {"ナ形容詞:ヤ列基本推量形",39}, {"ナ形容詞:ヤ列基本省略推量形",40}, {"ナ形容詞:ヤ列タ形",41}, {"ナ形容詞:ヤ列タ系推量形",42}, {"ナ形容詞:ヤ列タ系省略推量形",43}, {"ナ形容詞:ヤ列タ系条件形",44},
{"ナ形容詞:ヤ列タ系連用タリ形",45}, {"ナノ形容詞:語幹",1}, {"ナノ形容詞:基本形",2}, {"ナノ形容詞:ダ列基本連体形",3}, {"ナノ形容詞:ダ列特殊連体形",4},
{"ナノ形容詞:ダ列基本推量形",5}, {"ナノ形容詞:ダ列基本省略推量形",6}, {"ナノ形容詞:ダ列基本条件形",7}, {"ナノ形容詞:ダ列基本連用形",8}, {"ナノ形容詞:ダ列タ形",9}, {"ナノ形容詞:ダ列タ系推量形",10}, {"ナノ形容詞:ダ列タ系省略推量形",11}, {"ナノ形容詞:ダ列タ系条件形",12}, {"ナノ形容詞:ダ列タ系連用テ形",13}, {"ナノ形容詞:ダ列タ系連用タリ形",14}, {"ナノ形容詞:ダ列タ系連用ジャ形",15}, {"ナノ形容詞:ダ列文語連体形",16}, {"ナノ形容詞:ダ列文語条件形",17},
{"ナノ形容詞:デアル列基本形",18}, {"ナノ形容詞:デアル列命令形",19}, {"ナノ形容詞:デアル列基本推量形",20}, {"ナノ形容詞:デアル列基本省略推量形",21}, {"ナノ形容詞:デアル列基本条件形",22}, {"ナノ形容詞:デアル列基本連用形",23}, {"ナノ形容詞:デアル列タ形",24}, {"ナノ形容詞:デアル列タ系推量形",25}, {"ナノ形容詞:デアル列タ系省略推量形",26}, {"ナノ形容詞:デアル列タ系条件形",27}, {"ナノ形容詞:デアル列タ系連用テ形",28}, {"ナノ形容詞:デアル列タ系連用タリ形",29}, {"ナノ形容詞:デス列基本形",30},
{"ナノ形容詞:デス列基本推量形",31}, {"ナノ形容詞:デス列基本省略推量形",32}, {"ナノ形容詞:デス列タ形",33}, {"ナノ形容詞:デス列タ系推量形",34}, {"ナノ形容詞:デス列タ系省略推量形",35}, {"ナノ形容詞:デス列タ系条件形",36}, {"ナノ形容詞:デス列タ系連用テ形",37}, {"ナノ形容詞:デス列タ系連用タリ形",38}, {"ナノ形容詞:ヤ列基本形",39}, {"ナノ形容詞:ヤ列基本推量形",40}, {"ナノ形容詞:ヤ列基本省略推量形",41}, {"ナノ形容詞:ヤ列タ形",42}, {"ナノ形容詞:ヤ列タ系推量形",43}, {"ナノ形容詞:ヤ列タ系省略推量形",44}, {"ナノ形容詞:ヤ列タ系条件形",45}, {"ナノ形容詞:ヤ列タ系連用タリ形",46}, {"ナ形容詞特殊:語幹",1},
{"ナ形容詞特殊:基本形",2}, {"ナ形容詞特殊:ダ列基本連体形",3}, {"ナ形容詞特殊:ダ列特殊連体形",4}, {"ナ形容詞特殊:ダ列基本推量形",5}, {"ナ形容詞特殊:ダ列基本省略推量形",6}, {"ナ形容詞特殊:ダ列基本条件形",7}, {"ナ形容詞特殊:ダ列基本連用形",8}, {"ナ形容詞特殊:ダ列特殊連用形",9}, {"ナ形容詞特殊:ダ列タ形",10}, {"ナ形容詞特殊:ダ列タ系推量形",11}, {"ナ形容詞特殊:ダ列タ系省略推量形",12}, {"ナ形容詞特殊:ダ列タ系条件形",13}, {"ナ形容詞特殊:ダ列タ系連用テ形",14}, {"ナ形容詞特殊:ダ列タ系連用タリ形",15}, {"ナ形容詞特殊:ダ列タ系連用ジャ形",16}, {"ナ形容詞特殊:ダ列文語連体形",17}, {"ナ形容詞特殊:ダ列文語条件形",18}, {"ナ形容詞特殊:デアル列基本形",19}, {"ナ形容詞特殊:デアル列命令形",20}, {"ナ形容詞特殊:デアル列基本推量形",21}, {"ナ形容詞特殊:デアル列基本省略推量形",22}, {"ナ形容詞特殊:デアル列基本条件形",23}, {"ナ形容詞特殊:デアル列基本連用形",24}, {"ナ形容詞特殊:デアル列タ形",25}, {"ナ形容詞特殊:デアル列タ系推量形",26}, {"ナ形容詞特殊:デアル列タ系省略推量形",27}, {"ナ形容詞特殊:デアル列タ系条件形",28}, {"ナ形容詞特殊:デアル列タ系連用テ形",29}, {"ナ形容詞特殊:デアル列タ系連用タリ形",30}, {"ナ形容詞特殊:デス列基本形",31}, {"ナ形容詞特殊:デス列基本推量形",32}, {"ナ形容詞特殊:デス列基本省略推量形",33}, {"ナ形容詞特殊:デス列タ形",34}, {"ナ形容詞特殊:デス列タ系推量形",35}, {"ナ形容詞特殊:デス列タ系省略推量形",36}, {"ナ形容詞特殊:デス列タ系条件形",37}, {"ナ形容詞特殊:デス列タ系連用テ形",38}, {"ナ形容詞特殊:デス列タ系連用タリ形",39}, {"ナ形容詞特殊:ヤ列基本形",40}, {"ナ形容詞特殊:ヤ列基本推量形",41}, {"ナ形容詞特殊:ヤ列基本省略推量形",42}, {"ナ形容詞特殊:ヤ列タ形",43}, {"ナ形容詞特殊:ヤ列タ系推量形",44}, {"ナ形容詞特殊:ヤ列タ系省略推量形",45}, {"ナ形容詞特殊:ヤ列タ系条件形",46}, {"ナ形容詞特殊:ヤ列タ系連用タリ形",47}, {"タル形容詞:語幹",1}, {"タル形容詞:基本形",2}, {"タル形容詞:基本連用形",3},
{"判定詞:語幹",1}, {"判定詞:基本形",2}, {"判定詞:ダ列基本連体形",3}, {"判定詞:ダ列特殊連体形",4}, {"判定詞:ダ列基本推量形",5},
{"判定詞:ダ列基本省略推量形",6}, {"判定詞:ダ列基本条件形",7}, {"判定詞:ダ列タ形",8}, {"判定詞:ダ列タ系推量形",9}, {"判定詞:ダ列タ系省略推量形",10},
{"判定詞:ダ列タ系条件形",11}, {"判定詞:ダ列タ系連用テ形",12}, {"判定詞:ダ列タ系連用タリ形",13}, {"判定詞:ダ列タ系連用ジャ形",14}, {"判定詞:デアル列基本形",15}, {"判定詞:デアル列命令形",16}, {"判定詞:デアル列基本推量形",17}, {"判定詞:デアル列基本省略推量形",18}, {"判定詞:デアル列基本条件形",19},
{"判定詞:デアル列基本連用形",20}, {"判定詞:デアル列タ形",21}, {"判定詞:デアル列タ系推量形",22}, {"判定詞:デアル列タ系省略推量形",23}, {"判定詞:デアル列タ系条件形",24}, {"判定詞:デアル列タ系連用テ形",25}, {"判定詞:デアル列タ系連用タリ形",26}, {"判定詞:デス列基本形",27}, {"判定詞:デス列基本推量形",28},
{"判定詞:デス列基本省略推量形",29}, {"判定詞:デス列タ形",30}, {"判定詞:デス列タ系推量形",31}, {"判定詞:デス列タ系省略推量形",32}, {"判定詞:デス列タ系条件形",33}, {"判定詞:デス列タ系連用テ形",34}, {"判定詞:デス列タ系連用タリ形",35}, {"無活用型:語幹",1}, {"無活用型:基本形",2},
{"助動詞ぬ型:語幹",1}, {"助動詞ぬ型:基本形",2}, {"助動詞ぬ型:基本条件形",3}, {"助動詞ぬ型:基本連用形",4}, {"助動詞ぬ型:基本推量形",5},
{"助動詞ぬ型:基本省略推量形",6}, {"助動詞ぬ型:タ形",7}, {"助動詞ぬ型:タ系条件形",8}, {"助動詞ぬ型:タ系連用テ形",9}, {"助動詞ぬ型:タ系推量形",10},
{"助動詞ぬ型:タ系省略推量形",11}, {"助動詞ぬ型:音便基本形",12}, {"助動詞ぬ型:音便推量形",13}, {"助動詞ぬ型:音便省略推量形",14}, {"助動詞ぬ型:文語連体形",15}, {"助動詞ぬ型:文語条件形",16}, {"助動詞ぬ型:文語音便条件形",17}, {"助動詞だろう型:語幹",1}, {"助動詞だろう型:基本形",2},
{"助動詞だろう型:ダ列基本省略推量形",3}, {"助動詞だろう型:ダ列基本条件形",4}, {"助動詞だろう型:デアル列基本推量形",5}, {"助動詞だろう型:デアル列基本省略推量形",6}, {"助動詞だろう型:デス列基本推量形",7},
{"助動詞だろう型:デス列基本省略推量形",8}, {"助動詞だろう型:ヤ列基本推量形",9}, {"助動詞だろう型:ヤ列基本省略推量形",10}, {"助動詞そうだ型:語幹",1}, {"助動詞そうだ型:基本形",2}, {"助動詞そうだ型:ダ列タ系連用テ形",3}, {"助動詞そうだ型:デアル列基本形",4}, {"助動詞そうだ型:デス列基本形",5}, {"助動詞く型:語幹",1}, {"助動詞く型:基本形",2}, {"助動詞く型:基本連用形",3}, {"助動詞く型:文語連体形",4}, {"動詞性接尾辞ます型:語幹",1},
{"動詞性接尾辞ます型:基本形",2}, {"動詞性接尾辞ます型:未然形",3}, {"動詞性接尾辞ます型:意志形",4}, {"動詞性接尾辞ます型:省略意志形",5}, {"動詞性接尾辞ます型:命令形",6}, {"動詞性接尾辞ます型:タ形",7}, {"動詞性接尾辞ます型:タ系条件形",8}, {"動詞性接尾辞ます型:タ系連用テ形",9}, {"動詞性接尾辞ます型:タ系連用タリ形",10}, {"動詞性接尾辞うる型:語幹",1}, {"動詞性接尾辞うる型:基本形",2}, {"動詞性接尾辞うる型:基本条件形",3}};//}}}
}


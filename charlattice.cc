
#include "charlattice.h"
//static const 変数

namespace Morph {

int CharLattice::parse(std::string sent){//{{{
    size_t length = sent.size();

    auto CharNum = 0;
    node_list.clear();
    char_byte_length.clear();
    int pre_is_deleted = 0;
    int next_pre_is_deleted = 0;
    size_t next_pos=0;
    std::string last_char = "";

    for (size_t pos = 0; pos < length; pos+=next_pos) {
        unsigned char pos_chr = sent[pos];
        auto bytes = utf8_bytes(& pos_chr);
        char_byte_length.push_back(bytes);
        auto current_char = sent.substr(pos,bytes);
        //auto current_char_type = check_utf8_char_type((unsigned char *)current_char.c_str());
        //cout << current_char << endl;
            
        // 置き換え挿入の無い普通のコピー
        node_list.emplace_back();
        node_list[CharNum].emplace_back(current_char, OPT_NORMAL);
        next_pre_is_deleted = 0;
            
        if (current_char[0]&0x80) { /* 全角の場合 */
            //cerr << "<3byte char> " ;
            // (ーの文字 || (〜の文字 && (最後の文字である|| 次が記号 || 次がひらがな))) && ２文字目以降 //jumanの条件
            // (ーの文字 || 〜の文字) && (最後の文字である|| 次が記号 || 次がひらがな) && ２文字目以降 //旧条件
            // (ーの文字 || 〜の文字 || ３点リーダ)  && ２文字目以降 //現在の条件
            if ((current_char == DEF_PROLONG_SYMBOL1 || current_char == DEF_PROLONG_SYMBOL2 || 
                        false) && (pos > 0)){ //長音の母音化
                auto itr = prolonged_map.find(last_char); 
                if( itr != prolonged_map.end()){
                    // 長音を母音に置換してvectorの末尾でconstruct する
                    node_list[CharNum].emplace_back(itr->second, OPT_NORMALIZE | OPT_PROLONG_REPLACE);
                }
                //cerr << "prolong(rep) " ;
            } else { //小文字の大文字化
                auto itr = lower2upper.find(current_char);
                if(itr != lower2upper.end()){
                    node_list[CharNum].emplace_back(itr->second, OPT_NORMALIZE); 
                    //cerr << "prolong(capt) " ;
                }
            }
            next_pos = bytes;
        } else { //1byte文字
            next_pos = 1;
            //cerr << "<alpha>";
        }
            
        /* 長音挿入 (の削除)*/
        if (next_pos == BYTES4CHAR) { //全角文字
            //cerr << "<3byte> " << "pos:" << pos << " ";
            int pre_char_type = (pos > 0) ? check_utf8_char_type(last_char.c_str()) : -1;
            //cerr << "pre_char_type:" << pre_char_type << " ";
            //unsigned char next_chr = sent[pos+next_pos];
            auto post_char_type = check_utf8_char_type(sent.substr(pos+next_pos, utf8_bytes((unsigned char*)&sent[pos + next_pos])).c_str()); /* 文末の場合は0 */
                
            //直前の文字がある
            if ( pre_char_type > 0){ 
                //cerr << "<not first>";
                /* 長音記号で, 直前が削除されたか、直前が平仮名、直前が漢字かつ直後が平仮名 */
                if (((pre_is_deleted || pre_char_type == TYPE_HIRAGANA || (pre_char_type == TYPE_KANJI && post_char_type == TYPE_HIRAGANA)) &&
                        (current_char == DEF_PROLONG_SYMBOL1 || current_char == DEF_PROLONG_SYMBOL2 
                         // || current_char == DEF_ELIPSIS_SYMBOL1 || current_char == DEF_ELIPSIS_SYMBOL2
                         )) || 
                        /* 直前が削除されていて、現在文字が"っ"、かつ、直後が文末もしくは記号の場合も削除 */
                        (pre_is_deleted && current_char == DEF_PROLONG_SYMBOL3 && (post_char_type == 0 || post_char_type == TYPE_SYMBOL))) {
                    //cerr << "hatsuon del" << endl;
                    node_list[CharNum].emplace_back("", OPT_PROLONG_DEL);
                    next_pre_is_deleted = 1;
                } else {
                    //cerr << "<check2> " << (lower_map.find(last_char) !=lower_map.end()) << "preisdel:" << pre_is_deleted << " ";
                    if ( (lower_map.find(last_char) !=lower_map.end() && lower_map.find(last_char)->second == current_char) ||
                            (pre_is_deleted && lower_list.find(current_char)!=lower_list.end() && current_char == last_char)){
                        //cerr << "<del_cont_prolong> ";
                        /* 直前の文字が対応する平仮名の小書き文字、 ("かぁ" > "か(ぁ)" :()内は削除の意味)
                         * または、削除された同一の小書き文字の場合は削除 ("か(ぁ)ぁ" > "か(ぁぁ)") */
                        node_list[CharNum].emplace_back("", OPT_PROLONG_DEL); 
                        next_pre_is_deleted = 1;
                    }
                }
            } 
            //cerr << "<3byte end>" << endl;
        }
        //cerr << "end" << endl;
        last_char = current_char;
        pre_is_deleted = next_pre_is_deleted;
        CharNum++;
    }
    return 0;
}//}}}

// Dic クラスの中でやった方が良さそう？
std::vector<Darts::DoubleArray::result_pair_type> CharLattice::da_search_one_step( Darts::DoubleArray &da, int left_position, int right_position) {//{{{
    size_t current_da_node_pos;
    std::vector<CharNode>* left_char_node_list;
    //std::vector<CharNode>* right_char_node_list;
    std::vector<Darts::DoubleArray::result_pair_type> result; //value のみ, size は基本的に0
    //char *current_pat_buf; 
    //char current_node_type;
    
    if (left_position < 0) {
        // vector にする
        left_char_node_list = &(CharRootNodeList);
    } else {
        // todo:ラティスが構築されているかチェックを行う
        left_char_node_list = &(node_list[left_position]);
    }

    for(CharNode& left_char_node: (*left_char_node_list)){
        //cerr << "l:" << left_char_node.chr << "," << left_char_node.da_node_pos_num << endl; //0はありえない
        for (size_t i = 0; i < left_char_node.da_node_pos_num; i++) { /* 現在位置のtrieのノード群から */
            std::vector<CharNode>& right_char_node_list = node_list[right_position];
            for(CharNode& right_char_node: right_char_node_list){
                //cout << " r:" << right_char_node.chr << "," << right_char_node.da_node_pos_num << endl;
                if (right_char_node.chr[0] == '\0') { /* 削除ノード */
                    //cerr << "  da_del"<< endl;
                    if (left_position >= 0) {
                        right_char_node.node_type.push_back(left_char_node.node_type[i] | right_char_node.type);
                        right_char_node.da_node_pos.push_back( left_char_node.da_node_pos[i]);
                        size_t tmp=0;
                        int status = da.traverse( "" , left_char_node.da_node_pos[i], tmp ); //left_char_node.da_node_pos[i] の位置にあるものをそのまま読み込む
                            
                        if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
                            //cout << "    match:" << (status > 0) << endl;
                            //cout << "    " << left_char_node.chr << "." << right_char_node.chr << "(" << (status >> 8) << "," << (0xff & status) << ")" << endl;
                            Darts::DoubleArray::result_pair_type result_pair;
                            da.set_result(&result_pair, status, 0);
                            result.push_back( result_pair );
                        }
                        right_char_node.da_node_pos_num++;
                        if (MostDistantPosition < right_position) MostDistantPosition = right_position;
                    }
                } else { /* 通常ノード */
                    //cerr << "  da_normal" << endl;
                    if (!(right_char_node.type & OPT_DEVOICE || right_char_node.type & OPT_PROLONG_REPLACE) || 
                            (right_char_node.type & OPT_DEVOICE && left_position < 0) || /* 連濁ノードは先頭である必要がある */
                            (right_char_node.type & OPT_PROLONG_REPLACE && left_position >= 0)) { /* 長音置換ノードは先頭以外である必要がある */
                        //cout << "   da_ind: "<< i <<endl;
                            
                        //current_node_type = left_char_node.node_type[i] | right_char_node.type;
                        current_da_node_pos = left_char_node.da_node_pos[i];
                            
                        size_t tmp =0;
                        int status = da.traverse( right_char_node.chr , current_da_node_pos, tmp );
                        //cout << "   status:" << status << endl;
                        // -2 not found
                        // -1 found but no value (cont.)
                            
                        if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
                            //cout << "    match:" << (status > 0) << endl;
                            //cout << "    " << left_char_node.chr << "." << right_char_node.chr << "(" << (status >> 8) << "," << (0xff & status) << ")" << endl;
                            Darts::DoubleArray::result_pair_type result_pair;
                            da.set_result(&result_pair, status, 0);
                            result.push_back(result_pair); //返り値として保存
                        }

                        // マッチした場合も続きはあるかもしれない
                        if (status > 0 || status == -1) { /* マッチした場合と、ここではマッチしていないが、続きがある場合 */
                            //if(status == -1){
                            //    cout << "    continue:" << endl;
                            //}
                            right_char_node.node_type.push_back(left_char_node.node_type[i] | right_char_node.type);
                            right_char_node.da_node_pos.push_back( current_da_node_pos);
                            right_char_node.da_node_pos_num++;

                            if (MostDistantPosition < right_position)
                                MostDistantPosition = right_position;
                        }
                    }
                }
            }
        }
    }
    return result;
}//}}}

std::vector<Darts::DoubleArray::result_pair_type> CharLattice::da_search_from_position(Darts::DoubleArray &da,int position) {//{{{
    // TODO:lattice 構築済みかどうかのチェック
      
    std::vector<Darts::DoubleArray::result_pair_type> result;// result用
    /* initialization */
    CharLattice::MostDistantPosition = position - 1; //並列化の障害にもなるのでローカル変数にできないか？
    for (auto char_node = node_list.begin() + position /*?*/; char_node < node_list.end(); char_node++) {
        for(auto& cnode: *char_node) {
            cnode.da_node_pos.clear();
            cnode.node_type.clear();
            cnode.da_node_pos_num = 0;
        }
    }   
        
    // root から始まり，次の文字が node_list[position]のいずれかであるDAを探す.
    /* search double array by traverse */
    //cerr << position << " => " << MostDistantPosition  << endl;//文字
    unsigned char byte= char_byte_length[position];
    auto tokens_one_char = da_search_one_step(da, -1, position); 
    for(auto &pair: tokens_one_char){ //一文字のノード
        pair.length = byte; // マッチした表層のバイト数, マッチした辞書の情報は取り出していない. 
        result.push_back(pair);
    }
    for (size_t i = position + 1; i < node_list.size(); i++) {// 二文字
        byte += char_byte_length[i];
        if (MostDistantPosition < static_cast<int>(i) - 1) break; //position まで辿りつけた文字がない場合
        std::vector<Darts::DoubleArray::result_pair_type> tokens = da_search_one_step(da,i - 1, i);
        
        for(auto &pair: tokens){
            pair.length = byte; // マッチした表層のバイト数, マッチした辞書の情報は取り出していない. 
            //result に追加
            result.push_back(pair);
        }
    }
    return result;
}//}}}


//#define COPY_FOR_REFERENCE
//#ifndef COPY_FOR_REFERENCE
//
//
///*
//------------------------------------------------------------------------------
//	PROCEDURE: <register_nodes_by_deletion>
//------------------------------------------------------------------------------
//*/
//// 何をする関数？
//void register_nodes_by_deletion(char *src_buf, char *pat_buf, char node_type, char deleted_bytes) {
//    int length;
//    char *start_buf = src_buf, *current_pat_buf;
//    deleted_bytes++;
//
//    while (*src_buf) {
//        if (*src_buf == '\n') {//一行ごとにpat_buf に書き込む
//            current_pat_buf = pat_buf + strlen(pat_buf); /* 次のノードの最初 */
//            length = src_buf - start_buf + 1;
//            strncat(pat_buf, start_buf, length); /* 次のノードをコピー */
//            *current_pat_buf = node_type + PAT_BUF_INFO_BASE; /* node_typeの書き換え */
//            *(current_pat_buf + 1) = deleted_bytes + PAT_BUF_INFO_BASE; /* 削除バイト数 (取り出すときに1引く必要がある) */
//            *(current_pat_buf + length) = '\0';
//            start_buf = src_buf + 1;
//        }
//        src_buf++;
//    }
//}
//
//
///*
//------------------------------------------------------------------------------
//	PROCEDURE: <da_search_one_step>
//------------------------------------------------------------------------------
//*/
//
//// dic_no は 辞書の番号, 辞書が１つの場合気にする必要はなさそう. 辞書を分ける可能性は考慮する？
//
//// left_posisition が現在の文字，right_position が次の文字 ノードを探す?
//// 幅無制限のbeam searchの1step みたいな挙動．left のノードからright に続くda を探し，削除ノードならコピーし，次のノード集合を得る
//void da_search_one_step(int dic_no, int left_position, int right_position, char *pat_buf) {
//    int i, status;
//    size_t current_da_node_pos;
//    CHAR_NODE *left_char_node, *right_char_node;
//    char *current_pat_buf, current_node_type;
//
//#ifdef DEBUG
//    printf(";; S from node=%d, chr=%d, most_distant=%d\n", left_position, right_position, MostDistantPosition);
//#endif
//
//    if (left_position < 0) {
//        left_char_node = &CharRootNode;
//    }
//    else
//        left_char_node = &(CharLattice[left_position]);
//    
//    while (left_char_node) {
//        for (i = 0; i < left_char_node->da_node_pos_num; i++) { /* 現在位置のtrieのノード群から */
//            right_char_node = &(CharLattice[right_position]);
//            while (right_char_node) { /* 次の文字の集合 */
//                if (right_char_node->chr[0] == '\0') { /* 削除ノード */
//                    if (left_position >= 0) {
//                        // printf(";; D from node=%d, chr=%d\n", left_position, right_position + 1);
//                        // 今の文字の情報をコピーしておく
//                        right_char_node->node_type[right_char_node->da_node_pos_num] = left_char_node->node_type[i] | right_char_node->type;
//                        right_char_node->deleted_bytes[right_char_node->da_node_pos_num] = left_char_node->deleted_bytes[i] + strlen(CharLattice[right_position].chr);
////                        if (left_char_node->p_buffer[i]) { // p_buffer には何が入っている？
////                            if (right_char_node->p_buffer[right_char_node->da_node_pos_num] == NULL) {//初期化して
////                                right_char_node->p_buffer[right_char_node->da_node_pos_num] = (char *)malloc(50000);
////                                right_char_node->p_buffer[right_char_node->da_node_pos_num][0] = '\0';
////                            }
////                            strcat(right_char_node->p_buffer[right_char_node->da_node_pos_num], left_char_node->p_buffer[i]);//コピーする
////                            
////                            /* 各ノードのp_bufferの先頭(node_type)は未更新 -> pat_bufに入れるときに書き換える */
////                            // 登録？ p_buffer の中身を pat_buf にコピー
////                            register_nodes_by_deletion(left_char_node->p_buffer[i], pat_buf, 
////                                                       right_char_node->node_type[right_char_node->da_node_pos_num], 
////                                                       right_char_node->deleted_bytes[right_char_node->da_node_pos_num]);
////#ifdef DEBUG
////                            printf("*** D p_buf=%d p_buffer=%d at %d,", strlen(pat_buf), strlen(right_char_node->p_buffer[right_char_node->da_node_pos_num]), right_position);
////                            int j;
////                            for (j = 0; j < right_char_node->da_node_pos_num; j++)
////                                printf("%d,", right_char_node->da_node_pos[j]);
////                            printf("%d\n", left_char_node->da_node_pos[i]);
////#endif
////                        }
//                        right_char_node->da_node_pos[right_char_node->da_node_pos_num++] = left_char_node->da_node_pos[i];
//                        if (MostDistantPosition < right_position)
//                            MostDistantPosition = right_position;
//                    }
//                } else { /* 通常ノード */
//                    if (!(right_char_node->type & OPT_DEVOICE || right_char_node->type & OPT_PROLONG_REPLACE) || 
//                        (right_char_node->type & OPT_DEVOICE && left_position < 0) || /* 連濁ノードは先頭である必要がある */
//                        (right_char_node->type & OPT_PROLONG_REPLACE && left_position >= 0)) { /* 長音置換ノードは先頭以外である必要がある */
//                        current_node_type = left_char_node->node_type[i] | right_char_node->type;
//                        current_da_node_pos = left_char_node->da_node_pos[i];
//#ifdef DEBUG
//                        printf(";; T <%s> from %d ", right_char_node->chr, current_da_node_pos);
//#endif
//                        current_pat_buf = pat_buf + strlen(pat_buf);
//                        status = da_traverse(dic_no, right_char_node->chr, &current_da_node_pos, 0, strlen(right_char_node->chr), 
//                                             current_node_type, left_char_node->deleted_bytes[i], pat_buf);
//                        if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
//                            if (right_char_node->p_buffer[right_char_node->da_node_pos_num] == NULL) {
//                                right_char_node->p_buffer[right_char_node->da_node_pos_num] = (char *)malloc(50000);
//                                right_char_node->p_buffer[right_char_node->da_node_pos_num][0] = '\0';
//                            }
//                            strcat(right_char_node->p_buffer[right_char_node->da_node_pos_num], current_pat_buf);
//#ifdef DEBUG
//                            printf("OK (position=%d, value exists(p_buffer=%d).)", current_da_node_pos, strlen(right_char_node->p_buffer[right_char_node->da_node_pos_num]));
//#endif
//                        }
//                        else if (status == -1) {
//#ifdef DEBUG
//                            printf("OK (position=%d, cont..)", current_da_node_pos);
//#endif
//                            ;
//                        }
//                        if (status > 0 || status == -1) { /* マッチした場合と、ここではマッチしていないが、続きがある場合 */
//                            right_char_node->node_type[right_char_node->da_node_pos_num] = current_node_type; /* ノードタイプの伝播 */
//                            right_char_node->deleted_bytes[right_char_node->da_node_pos_num] = left_char_node->deleted_bytes[i]; /* 削除文字数の伝播 */
//                            right_char_node->da_node_pos[right_char_node->da_node_pos_num++] = current_da_node_pos;
//                            if (MostDistantPosition < right_position)
//                                MostDistantPosition = right_position;
//                        }
//#ifdef DEBUG
//                        printf("\n");
//#endif
//                    }
//                }
//                if (right_char_node->da_node_pos_num >= MAX_NODE_POS_NUM) {
//#ifdef DEBUG
//                    fprintf(stderr, ";; exceeds MAX_NODE_POS_NUM in %s\n", String);
//#endif
//                    right_char_node->da_node_pos_num = MAX_NODE_POS_NUM - 1;
//                }
//                right_char_node = right_char_node->next;
//            }
//        }
//        left_char_node = left_char_node->next;
//    }
//}
//
///*
//------------------------------------------------------------------------------
//	PROCEDURE: <da_search_from_position>
//------------------------------------------------------------------------------
//*/
//void da_search_from_position(int dic_no, int position, char *pat_buf)
//{
//    int i, j;
//    CHAR_NODE *char_node;
//
//#ifdef DEBUG
//    printf(";; SS from start_position=%d\n", position);
//#endif
//
//    /* initialization */
//    MostDistantPosition = position - 1;
//    for (i = position; i < CharNum; i++) {
//        char_node = &(CharLattice[i]);
//        while (char_node) {
//            for (j = 0; j < char_node->da_node_pos_num; j++) {
//                if (char_node->p_buffer[j])
//                    free(char_node->p_buffer[j]);
//                char_node->p_buffer[j] = NULL;
//            }
//            char_node->da_node_pos_num = 0;
//            char_node = char_node->next;
//        }
//    }
//
//    /* search double array by traverse */
//    //root から始まり，次の文字が char_node[position] であるDAを探す.
//    da_search_one_step(dic_no, -1, position, pat_buf); // the second -1 means the search from the root of double array
//    for (i = position + 1; i < CharNum; i++) {
//        if (MostDistantPosition < i - 1)
//            break;
//        // 二文字
//        da_search_one_step(dic_no, i - 1, i, pat_buf);
//    }
//}
//
//
//// オノマトペなので char lattice とはあまり関係ない
//
///*
//------------------------------------------------------------------------------
//        PROCEDURE: <recognize_repetition>   >>> Added by Ryohei Sasano <<<
//------------------------------------------------------------------------------
//*/
//int recognize_onomatopoeia(int pos)
//{
//    int i, len, code, next_code;
//    int key_length = strlen(String + pos); /* キーの文字数を数えておく */
//
//    /* 通常の平仮名、片仮名以外から始まるものは不可 */
//    code = check_code(String, pos);
//    if (code != HIRAGANA && code != KATAKANA) return FALSE;
//    for (i = 0; *lowercase[i]; i++) {
//	if (!strncmp(String + pos, lowercase[i], BYTES4CHAR)) return FALSE;
//    }
//
//    /* 反復型オノマトペ */
//    if (Repetition_Opt) {
//        for (len = 2; len < 5; len++) {
//            /* 途中で文字種が変わるものは不可 */
//            next_code = check_code(String, pos + len * BYTES4CHAR - BYTES4CHAR);
//            if (next_code == CHOON) next_code = code; /* 長音は直前の文字種として扱う */
//            if (key_length < len * 2 * BYTES4CHAR || code != next_code) break;
//            code = next_code;
//
//            /* 反復があるか判定 */
//            if (strncmp(String + pos, String + pos + len * BYTES4CHAR, len * BYTES4CHAR)) continue;
//            /* ただし3文字が同じものは不可 */
//            if (!strncmp(String + pos, String + pos + BYTES4CHAR, BYTES4CHAR) &&
//                    !strncmp(String + pos, String + pos + 2 * BYTES4CHAR, BYTES4CHAR)) continue;
//
//            m_buffer[m_buffer_num].hinsi = onomatopoeia_hinsi;
//            m_buffer[m_buffer_num].bunrui = onomatopoeia_bunrui;
//            m_buffer[m_buffer_num].con_tbl = onomatopoeia_con_tbl;
//
//            m_buffer[m_buffer_num].katuyou1 = 0;
//            m_buffer[m_buffer_num].katuyou2 = 0;
//            m_buffer[m_buffer_num].length = len * 2 * BYTES4CHAR;
//
//            strncpy(m_buffer[m_buffer_num].midasi, String+pos, len * 2 * BYTES4CHAR);
//            m_buffer[m_buffer_num].midasi[len * 2 * BYTES4CHAR] = '\0';
//            strncpy(m_buffer[m_buffer_num].yomi, String+pos, len * 2 * BYTES4CHAR);
//            m_buffer[m_buffer_num].yomi[len * 2 * BYTES4CHAR] = '\0';
//
//            /* weightの設定 */
//            m_buffer[m_buffer_num].weight = REPETITION_COST * len;
//            /* 拗音を含む場合 */
//            for (i = CONTRACTED_LOWERCASE_S; i < CONTRACTED_LOWERCASE_E; i++) {
//                if (strstr(m_buffer[m_buffer_num].midasi, lowercase[i])) break;
//            }
//            if (i < CONTRACTED_LOWERCASE_E) {
//                if (len == 2) continue; /* 1音の繰り返しは禁止 */		
//                /* 1文字分マイナス+ボーナス */
//                m_buffer[m_buffer_num].weight -= REPETITION_COST + CONTRACTED_BONUS;
//            }
//            /* 濁音・半濁音を含む場合 */
//            for (i = 0; *dakuon[i]; i++) {
//                if (strstr(m_buffer[m_buffer_num].midasi, dakuon[i])) break;
//            }
//            if (*dakuon[i]) {
//                m_buffer[m_buffer_num].weight -= DAKUON_BONUS; /* ボーナス */
//                /* 先頭が濁音の場合はさらにボーナス */
//                if (!strncmp(m_buffer[m_buffer_num].midasi, dakuon[i], BYTES4CHAR)) 
//                    m_buffer[m_buffer_num].weight -= DAKUON_BONUS;
//            }
//            /* カタカナである場合 */
//            if (code == KATAKANA) 
//                m_buffer[m_buffer_num].weight -= KATAKANA_BONUS;
//
//            strcpy(m_buffer[m_buffer_num].midasi2, m_buffer[m_buffer_num].midasi);
//            strcpy(m_buffer[m_buffer_num].imis, "\"");
//            strcat(m_buffer[m_buffer_num].imis, DEF_ONOMATOPOEIA_IMIS);
//            strcat(m_buffer[m_buffer_num].imis, "\"");
//
//            check_connect(pos, m_buffer_num, 0);
//            if (++m_buffer_num == mrph_buffer_max) realloc_mrph_buffer();	
//            break; /* 最初にマッチしたもののみ採用 */
//        }
//    }
//}
//
//
//#endif
}

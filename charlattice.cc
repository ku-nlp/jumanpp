
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
                if (((pre_is_deleted || pre_char_type == TYPE_KATAKANA || pre_char_type == TYPE_HIRAGANA || (pre_char_type == TYPE_KANJI && post_char_type == TYPE_HIRAGANA)) &&
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
    constructed = true;
    return 0;
}//}}}

// Dic クラスの中でやった方が良さそう？
std::vector<CharLattice::da_result_pair_type> CharLattice::da_search_one_step( Darts::DoubleArray &da, int left_position, int right_position) {//{{{
    size_t current_da_node_pos;
    std::vector<CharNode>* left_char_node_list;
    //std::vector<CharNode>* right_char_node_list;
    
    //value と len の２つの要素がある構造体のvector
    // value の上位24 bit は トークンが存在するDA上の位置
    // value の下位 8 bit は 得られたトークンの数を表す
    // len は流用されていて，Node のフラグを表している
    std::vector<da_result_pair_type> result;
    //char *current_pat_buf; 
    //char current_node_type;
    
    if (left_position < 0) {
        // vector にする
        left_char_node_list = &(CharRootNodeList);
    } else {
        // TODO:ラティスが構築されているかチェックを行う
        left_char_node_list = &(node_list[left_position]);
    }

    for(CharNode& left_char_node: (*left_char_node_list)){
        //cerr << "l:" << left_char_node.chr << "," << left_char_node.da_node_pos_num << endl; //0はありえない
        for (size_t i = 0; i < left_char_node.da_node_pos_num; i++) { /* 現在位置のtrieのノード群から */
            std::vector<CharNode>& right_char_node_list = node_list[right_position];
            for(CharNode& right_char_node: right_char_node_list){
                //cout << " r:" << right_char_node.chr << "," << right_char_node.da_node_pos_num << endl;
                if (right_char_node.chr[0] == '\0') { /* 削除ノード */
                    if (left_position >= 0) {
                        right_char_node.node_type.push_back(left_char_node.node_type[i] | right_char_node.type | OPT_PROLONG_DEL_LAST); // このタイプが使われるのは何処？
                        right_char_node.da_node_pos.push_back( left_char_node.da_node_pos[i]);
                        size_t tmp=0;
                        int status = da.traverse("" , left_char_node.da_node_pos[i], tmp); //left_char_node.da_node_pos[i] の位置にあるものをそのまま読み込む
                            
                        if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
                            //cout << "    match:" << (status > 0) << endl;
                            //cout << "    " << left_char_node.chr << "." << right_char_node.chr << "(" << (((unsigned int)status) >> 8) << "," << (0xff & status) << ")" 
                            //     << "stat: "<<  (left_char_node.node_type[i] | (right_char_node.type & (~OPT_PROLONG_DEL)) | OPT_PROLONG_DEL_LAST) << endl;
                            da_result_pair_type result_pair;
                            //da.set_result(&result_pair, status, (left_char_node.node_type[i] | (right_char_node.type & (~OPT_PROLONG_DEL)) | OPT_PROLONG_DEL_LAST));
                            result.emplace_back(da_result_pair_type(status,0,(left_char_node.node_type[i] | (right_char_node.type & (~OPT_PROLONG_DEL)) | OPT_PROLONG_DEL_LAST)));
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
                        int status = da.traverse( right_char_node.chr, current_da_node_pos, tmp );
                        //cout << "   status:" << status << endl;
                        // -2 not found
                        // -1 found but no value (cont.)
                            
                        if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
                            //cout << "    match:" << (status > 0) << endl;
                            //cout << "    " << left_char_node.chr << "." << right_char_node.chr << "(" << (status >> 8) << "," << (0xff & status) << ")" << " stat:" << (left_char_node.node_type[i] | right_char_node.type) << endl;
                            da_result_pair_type result_pair;
                            //da.set_result(&result_pair, status, left_char_node.node_type[i] | right_char_node.type);
                            result.emplace_back(status,0,left_char_node.node_type[i] | right_char_node.type); //返り値
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

std::vector<CharLattice::da_result_pair_type> CharLattice::da_search_from_position(Darts::DoubleArray &da,int position) {//{{{
    std::vector<da_result_pair_type> result;
    if (!constructed)
        return result;
    /* initialization */
    CharLattice::MostDistantPosition = position - 1; //並列化の障害にもなるのでローカル変数にできないか？
    for (auto char_node = node_list.begin() + position /*?*/; char_node < node_list.end(); char_node++) {
        for(auto& cnode: *char_node) {
            cnode.da_node_pos.clear();
            cnode.node_type.clear();
            cnode.da_node_pos_num = 0;
        }
    }
        
    // root から始まり，次の文字が node_list[position] のいずれかであるDAを探す.
    /* search double array by traverse */
    // cerr << position << " => " << MostDistantPosition  << endl;//文字
    unsigned char byte= char_byte_length[position];
    auto tokens_one_char = da_search_one_step(da, -1, position); 
    for(auto &pair: tokens_one_char){ //一文字のノード
        std::get<1>(pair) = byte; // マッチした表層のバイト数, マッチした辞書の情報は取り出していない. 
        result.push_back(pair);
    }
    for (size_t i = position + 1; i < node_list.size(); i++) {// 二文字
        byte += char_byte_length[i];
        if (MostDistantPosition < static_cast<int>(i) - 1) break; //position まで辿りつけた文字がない場合
        std::vector<da_result_pair_type> tokens = da_search_one_step(da,i - 1, i);
         
        for(auto &pair: tokens){
            std::get<1>(pair) = byte;
            result.push_back(pair);
        }
    }
    return result;
}//}}}

}


#include "charlattice.h"
#include "util/logging.hpp"
#include <iostream>
// static const 変数

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

    bool CharLattice::initialized = false;

    const char32_t DEF_PROLONG_SYMBOL1{u'ー'};
    const char32_t DEF_PROLONG_SYMBOL2{u'〜'};
    const char32_t DEF_PROLONG_SYMBOL3{u'っ'};
    const char32_t DEF_ELIPSIS_SYMBOL1{u'…'}; // HORIZONTAL ELLIPSIS
    const char32_t DEF_ELIPSIS_SYMBOL2{u'⋯'}; // MIDDLE_HORIZONTAL ELLIPSIS

    /* 小書き文字・拗音(+"ん","ン")、小書き文字に対応する大文字の一覧
       非正規表記の処理(小書き文字・対応する大文字)、
       オノマトペ認識(開始文字チェック、拗音(cf. CONTRACTED_BONUS))で利用 */
    const util::FlatSet<char32_t> lowercase{
        u'ぁ', u'ぃ', u'ぅ', u'ぇ', u'ぉ', u'ゎ', u'ヵ', u'ァ', u'ィ', u'ゥ', u'ェ', u'ォ',
        u'ヮ', u'っ', u'ッ', u'ん', u'ン', u'ゃ', u'ャ', u'ゅ', u'ュ', u'ょ', u'ョ'};
    const util::FlatSet<char32_t> uppercase{u'あ', u'い', u'う', u'え',
                                             u'お', u'わ', u'か'};
    //lower list
    const util::FlatSet<char32_t> lower_list{u'ぁ', u'ぃ', u'ぅ', u'ぇ', u'ぉ'};
    /* 長音記号直前の文字が pre_prolonged[i] だった場合、長音記号を
     * prolonged2chr[i] に置換 */
    const util::FlatSet<char32_t> pre_prolonged{
        u'か', u'ば', u'ま', u'ゃ',                               /* あ */
        u'い', u'き', u'し', u'ち', u'に', u'ひ', u'じ', u'け', u'せ', /* い */
        u'へ', u'め', u'れ', u'げ', u'ぜ', u'で', u'べ', u'ぺ', u'く',
        u'す', u'つ', u'ふ', u'ゆ', u'ぐ', u'ず', u'ぷ', u'ゅ', /* う */
        u'お', u'こ', u'そ', u'と', u'の', u'ほ', u'も', u'よ', u'ろ',
        u'ご', u'ぞ', u'ど', u'ぼ', u'ぽ', u'ょ', u'え', u'ね', }; /* え(ね) */
    // 0
    const util::FlatSet<char32_t> prolonged2chr{
        u'あ', u'あ', u'あ', u'あ',                               /* あ */
        u'い', u'い', u'い', u'い', u'い', u'い', u'い', u'い', u'い', /* い */
        u'い', u'い', u'い', u'い', u'い', u'い', u'い', u'い', u'う',
        u'う', u'う', u'う', u'う', u'う', u'う', u'う', u'う', /* う */
        u'う', u'う', u'う', u'う', u'う', u'う', u'う', u'う', u'う',
        u'う', u'う', u'う', u'う', u'う', u'う', u'え', u'え', }; /* え(ね) */
    // 0
    // ねぇ，ねえ，が除外されている理由は？

    const util::FlatSet<char32_t> pre_lower{
        u'か', u'さ', u'た', u'な', u'は', u'ま', u'や', u'ら', u'わ',
        u'が', u'ざ', u'だ', u'ば', u'ぱ',                         /* ぁ:14 */
        u'い', u'し', u'に', u'り', u'ぎ', u'じ', u'ね', u'れ', u'ぜ', /* ぃ: 9 */
        u'う', u'く', u'す', u'ふ', u'む', u'る', u'よ',             /* ぅ: 7 */
        u'け', u'せ', u'て', u'め', u'れ', u'ぜ', u'で',             /* ぇ: 7 */
        u'こ', u'そ', u'の', u'も', u'よ', u'ろ', u'ぞ', u'ど'};      /* ぉ: 8 */
    
    const util::FlatMap<char32_t, char32_t> lower_map{
        {u'か', u'ぁ'}, {u'さ', u'ぁ'}, {u'た', u'ぁ'}, {u'な', u'ぁ'}, {u'は', u'ぁ'},
        {u'ま', u'ぁ'}, {u'や', u'ぁ'}, {u'ら', u'ぁ'}, {u'わ', u'ぁ'}, {u'が', u'ぁ'},
        {u'ざ', u'ぁ'}, {u'だ', u'ぁ'}, {u'ば', u'ぁ'}, {u'ぱ', u'ぁ'}, {u'い', u'ぃ'},
        {u'し', u'ぃ'}, {u'に', u'ぃ'}, {u'り', u'ぃ'}, {u'ぎ', u'ぃ'}, {u'じ', u'ぃ'},
        {u'ね', u'ぃ'}, {u'れ', u'ぃ'}, {u'ぜ', u'ぃ'}, {u'う', u'ぅ'}, {u'く', u'ぅ'},
        {u'す', u'ぅ'}, {u'ふ', u'ぅ'}, {u'む', u'ぅ'}, {u'る', u'ぅ'}, {u'よ', u'ぅ'},
        {u'け', u'ぇ'}, {u'せ', u'ぇ'}, {u'て', u'ぇ'}, {u'め', u'ぇ'}, {u'れ', u'ぇ'},
        {u'ぜ', u'ぇ'}, {u'で', u'ぇ'}, {u'こ', u'ぉ'}, {u'そ', u'ぉ'}, {u'の', u'ぉ'},
        {u'も', u'ぉ'}, {u'よ', u'ぉ'}, {u'ろ', u'ぉ'}, {u'ぞ', u'ぉ'}, {u'ど', u'ぉ'}};

    const util::FlatMap<char32_t, const char*> CharLattice::lower2upper_def{
        {u'ぁ', "あ"}, {u'ぃ', "い"}, {u'ぅ', "う"}, {u'ぇ', "え"},
        {u'ぉ', "お"}, {u'ゎ', "わ"}, {u'ヶ', "ケ"}, {u'ケ', "ヶ"}};
    util::FlatMap<char32_t, Codepoint> CharLattice::lower2upper;

    /* 長音置換のルールで利用 */
    const util::FlatMap<char32_t, const char*> CharLattice::prolonged_map_def{
        {u'か', "あ"}, {u'ば', "あ"}, {u'ま', "あ"}, {u'ゃ', "あ"}, {u'い', "い"},
        {u'き', "い"}, {u'し', "い"}, {u'ち', "い"}, {u'に', "い"}, {u'ひ', "い"},
        {u'じ', "い"}, {u'け', "い"}, {u'せ', "い"}, {u'へ', "い"}, {u'め', "い"},
        {u'れ', "い"}, {u'げ', "い"}, {u'ぜ', "い"}, {u'で', "い"}, {u'べ', "い"},
        {u'ぺ', "い"}, {u'く', "う"}, {u'す', "う"}, {u'つ', "う"}, {u'ふ', "う"},
        {u'ゆ', "う"}, {u'ぐ', "う"}, {u'ず', "う"}, {u'ぷ', "う"}, {u'ゅ', "う"},
        {u'お', "う"}, {u'こ', "う"}, {u'そ', "う"}, {u'と', "う"}, {u'の', "う"},
        {u'ほ', "う"}, {u'も', "う"}, {u'よ', "う"}, {u'ろ', "う"}, {u'ご', "う"},
        {u'ぞ', "う"}, {u'ど', "う"}, {u'ぼ', "う"}, {u'ぽ', "う"}, {u'ょ', "う"},
        {u'え', "え"}, {u'ね', "え"}};
    
    util::FlatMap<char32_t, Codepoint> CharLattice::prolonged_map;


// 長音記号で, 直前が削除されたか、直前が平仮名、直前が漢字かつ直後が平仮名
inline bool check_removable_choon(bool pre_is_deleted, CharacterClass pre_char_type, CharacterClass post_char_type, const Codepoint& currentCp){
  return (( pre_is_deleted || IsCompatibleCharClass(pre_char_type, CharacterClass::KATAKANA) ||
            IsCompatibleCharClass(pre_char_type, CharacterClass::HIRAGANA) ||
            ( IsCompatibleCharClass(pre_char_type, CharacterClass::KANJI) &&
              IsCompatibleCharClass(post_char_type, CharacterClass::HIRAGANA))) &&
          ( currentCp.codepoint == DEF_PROLONG_SYMBOL1 ||
            currentCp.codepoint == DEF_PROLONG_SYMBOL2));
}

// 直前が削除されていて、現在文字が"っ"、かつ、直後が文末もしくは記号の場合も削除
inline bool check_removable_hatsuon(bool pre_is_deleted, bool isLastCharacter, CharacterClass post_char_type, const Codepoint& currentCp){

  return (pre_is_deleted && currentCp.codepoint == DEF_PROLONG_SYMBOL3 &&
                     (isLastCharacter || IsCompatibleCharClass(post_char_type, CharacterClass::SYMBOL) ));
}

/* 直前の文字が対応する平仮名の小書き文字、 ("かぁ" >
 * "か(ぁ)" :()内は削除の意味)
 * または、削除された同一の小書き文字の場合は削除
 * ("か(ぁ)ぁ" > "か(ぁぁ)") */
inline bool check_remobable_youon(bool pre_is_deleted, const Codepoint& currentCp, const Codepoint& lastCp){
  // LOG_INFO() << "<check2> " << (lower_map.find(last_char)
  // !=lower_map.end()) << "preisdel:" << pre_is_deleted << "
  // ";
  auto itr = lower_map.find(lastCp.codepoint);
  return (( itr != lower_map.end() &&
            itr->second == currentCp.codepoint) ||
          ( pre_is_deleted &&
            lower_list.find(currentCp.codepoint) != lower_list.end() &&
            currentCp.codepoint == lastCp.codepoint));
}


int CharLattice::parse(const std::vector<Codepoint>& codepoints) { //{{{
    size_t length = codepoints.size();

    Codepoint skipped(StringPiece((const u8*)"", (size_t)0));

    auto CharNum = 0;
    node_list.clear(); // charlattice の lattice部
    node_list.resize(length);
    char_byte_length.clear(); //?
    int pre_is_deleted = 0;
    int next_pre_is_deleted = 0;
    size_t next_pos = 0;

    for (size_t pos = 0; pos < length; ++pos) {
        bool isFirstCharacter = pos == 0;
        bool isLastCharacter = pos+1==length;

        const Codepoint& currentCp = codepoints[pos];
        LOG_INFO() << "emplace current cp" << currentCp.bytes << "@" << pos << "\n";
        node_list[pos].emplace_back(currentCp, OptCharLattice::OPT_NORMAL);
        next_pre_is_deleted = 0;

        /* Double Width Characters */
        if (currentCp.hasClass(CharacterClass::FAMILY_DOUBLE)) { 
            // (ーの文字 || 〜の文字 || ３点リーダ)  && ２文字目以降
            if ((currentCp.codepoint == DEF_PROLONG_SYMBOL1 ||
                 currentCp.codepoint == DEF_PROLONG_SYMBOL2 ) &&
                !isFirstCharacter) { //長音の母音化
                auto itr = prolonged_map.find(codepoints[pos-1].codepoint);
                if (itr != prolonged_map.end()) {
                    // 長音を母音に置換してvectorの末尾でconstruct する
                    LOG_INFO() << "<substitute_choon_boin> " << currentCp.bytes << ">" << itr->second.bytes << "@" << pos << "\n";
                    node_list[pos].emplace_back( itr->second, OptCharLattice::OPT_PROLONG_REPLACED );
                }
            } else { //小文字の大文字化
                auto itr = lower2upper.find(currentCp.codepoint);
                if (itr != lower2upper.end()) {
                    LOG_INFO() << "<substitute_small_large> " << currentCp.bytes << ">" << itr->second.bytes << "@" << pos << "\n";
                    node_list[pos].emplace_back( itr->second, OptCharLattice::OPT_NORMALIZE);
                }
            }
        }

        /* 長音削除 */
        if (currentCp.hasClass(CharacterClass::FAMILY_DOUBLE)  ) { // Not last character
            const Codepoint& next_cp = codepoints[pos+1];

            CharacterClass pre_char_type =
                (!isFirstCharacter) ? codepoints[pos-1].charClass : CharacterClass::FAMILY_OTHERS;
            auto post_char_type = codepoints[pos+1].charClass;

            //直前の文字がある
            if (!isFirstCharacter) {
                auto lastCp = codepoints[pos-1];
                if ( (check_removable_choon(pre_is_deleted, pre_char_type, post_char_type, currentCp)) ||
                    check_removable_hatsuon(pre_is_deleted, isLastCharacter, post_char_type, currentCp)) {
                    LOG_INFO() << "<del_prolong> " << currentCp.bytes << "@" << pos << "\n";
                    node_list[pos].emplace_back(skipped, OptCharLattice::OPT_PROLONG_DEL);
                    next_pre_is_deleted = 1;
                } else {
                    if (check_remobable_youon(pre_is_deleted, currentCp, lastCp)) {
                        LOG_INFO() << "<del_youon> @" << pos << "\n";
                        node_list[pos].emplace_back(skipped, OptCharLattice::OPT_PROLONG_DEL);
                        next_pre_is_deleted = 1;
                    }
                }
            }
        }
        pre_is_deleted = next_pre_is_deleted;
    }
    constructed = true;
    return 0;
} //}}}

std::vector<CharLattice::da_trie_result> CharLattice::da_search_one_step(int left_position, int right_position) { //{{{
    std::vector<CharLattice::da_trie_result> result;  
    
    std::vector<CharNode> *left_char_node_list;
    if (left_position < 0) {
        left_char_node_list = &(CharRootNodeList);
    } else {
        left_char_node_list = &(node_list[left_position]);
    }
    LOG_INFO() << "da_earch_one_step(" << left_position << ", " <<  right_position << ").nodesize = " <<  left_char_node_list->size()<<  "\n";

    size_t current_da_node_pos;
    auto trav = entries.traversal(); // root node

    for (CharNode &left_char_node : (*left_char_node_list)) {
        /* 現在位置のtrieのノード群から */
        // 同品詞 i: node_index
        for (size_t i = 0; i < left_char_node.da_node_pos.size(); ++i) { 
            // right_position: 次の文字位置
            std::vector<CharNode> &right_char_node_list = node_list[right_position];
            
            LOG_INFO() << "right_char_node.size() = "  << right_char_node_list.size() << "\n";
            for (CharNode &right_char_node : right_char_node_list) {
                LOG_INFO() << "right_char_node.cp = "  <<  right_char_node.cp.bytes << "\n";
                if (right_char_node.cp.codepoint == 0) { /* 削除ノード */
                    if (left_position >= 0) { // 最初の一文字はこの処理を行わない
                        // 現在の位置にあるものをそのまま読み込む
                        LOG_INFO() << "step by \"\" ";
                        auto status = trav.step("", left_char_node.da_node_pos[i]);

                        /* マッチしたら結果に登録する */
                        if (status == TraverseStatus::Ok) { 
                            auto dicEntries = trav.entries();
                            i32 ptr = 0;
                            while (dicEntries.readOnePtr(&ptr)) {
                              auto node_type = (left_char_node.node_type[i] |
                                 (right_char_node.type & (~(OptCharLattice::OPT_PROLONG_DEL))) |
                                 OptCharLattice::OPT_PROLONG_DEL_LAST);
                              EntryPtr eptr(ptr);
                              result.push_back(std::make_pair(eptr, node_type));
                            }
                            LOG_INFO() << "Traverse Ok" << "\n" ;
                        }else if (status == TraverseStatus::NoLeaf){
                            LOG_INFO() << "Traverse NoLeaf"  << "\n";
                        }else if (status == TraverseStatus::NoNode){
                            LOG_INFO() << "Traverse NoNode"  << "\n";
                        }
                         
                        // 次の探索開始位置に登録
                        right_char_node.node_type.push_back(
                            left_char_node.node_type[i] | right_char_node.type |
                            OptCharLattice::OPT_PROLONG_DEL_LAST); 
                        right_char_node.da_node_pos.push_back( left_char_node.da_node_pos[i]);

                        if (MostDistantPosition < right_position)
                            MostDistantPosition = right_position;
                    }
                } else { /* 通常ノード */
                    if ( (right_char_node.type & OptCharLattice::OPT_PROLONG_REPLACE) == OptCharLattice::OPT_INVALID ||
                        ( (right_char_node.type & OptCharLattice::OPT_PROLONG_REPLACE) != OptCharLattice::OPT_INVALID &&
                         left_position >= 0)) { 
                        // 長音置換ノードは先頭以外である必要がある
                        current_da_node_pos = left_char_node.da_node_pos[i];

                        LOG_INFO() << "step by " << right_char_node.cp.bytes;
                        auto status = trav.step(right_char_node.cp.bytes, current_da_node_pos);

                        if (status == TraverseStatus::Ok) { 
                            auto node_type = left_char_node.node_type[i] | right_char_node.type;
                            if( (node_type & OptCharLattice::OPT_NORMALIZED) != OptCharLattice::OPT_INVALID){
                                auto dicEntries = trav.entries();
                                i32 ptr = 0;
                                while (dicEntries.readOnePtr(&ptr)) {
                                  EntryPtr eptr(ptr);
                                  result.push_back(std::make_pair(eptr, node_type));
                                }
                            }
                            LOG_INFO() << "Traverse Ok" << "\n" ;
                        }else if (status == TraverseStatus::NoLeaf){
                            LOG_INFO() << "Traverse NoLeaf"  << "\n";
                        }else if (status == TraverseStatus::NoNode){
                            LOG_INFO() << "Traverse NoNode"  << "\n";
                        }


                        //* マッチした場合と、ここではマッチしていないが、続きがある場合
                        if (status == TraverseStatus::Ok ||
                            status == TraverseStatus::NoLeaf) { 
                            
                            // 開始位置位置として追加
                            right_char_node.node_type.push_back( left_char_node.node_type[i] | right_char_node.type);
                            right_char_node.da_node_pos.push_back( current_da_node_pos );

                            if (MostDistantPosition < right_position)
                                MostDistantPosition = right_position;
                        }
                    }
                }
            }
        }
    }
    return result;
} //}}}

//std::vector<CharLattice::da_result_pair_type>
std::vector<CharLattice::CLResult> CharLattice::da_search_from_position(size_t position) { //{{{
    std::vector<CharLattice::CLResult> result;
    if (!constructed) return result;

    /* initialization */
    CharLattice::MostDistantPosition = position - 1; 
    for (auto char_node = node_list.begin() + position /*?*/;
         char_node < node_list.end(); char_node++) {
        for (auto &cnode : *char_node) {
            cnode.da_node_pos.clear();
            cnode.node_type.clear();
        }
    }

    auto tokens_one_char = da_search_one_step(-1, position);
    for (auto &pair : tokens_one_char) { //一文字のノード
        auto type = pair.second;
        result.push_back(std::make_tuple(pair.first, type, position, position+1));
    }

    for (size_t i = position + 1; i < node_list.size(); i++) { // 二文字以上
        if (MostDistantPosition < static_cast<int>(i) - 1)
            break; // position まで辿りつけた文字がない場合
        LOG_INFO() << "da_search @ " << position << "," <<  i << "\n";
        auto tokens = da_search_one_step(i - 1, i);

        for (auto &pair : tokens) {
            auto type = pair.second;
            for (size_t j = position; j< i+1;j++){
                LOG_INFO() << node_list[j][0].cp.bytes;
            }
            LOG_INFO() << " range(" << position << ", "<< i << ")" << "\n";
            result.push_back(std::make_tuple(pair.first, type, position, i+1 ));
        }
    }
    return result;
} //}}}
}
}
}
}

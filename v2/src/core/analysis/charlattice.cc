
#include "charlattice.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

const char32_t DEF_PROLONG_SYMBOL1{u'ー'};
const char32_t DEF_PROLONG_SYMBOL2{u'〜'};
const char32_t DEF_PROLONG_SYMBOL3{u'っ'};

struct CharacterMap{
  using Codepoint = jumanpp::chars::InputCodepoint;
  using CharacterClass = jumanpp::chars::CharacterClass;
  using CodepointStorage = std::vector<Codepoint>;
  using P = std::pair<char32_t, const char* >;

  inline static Codepoint toCodepoint(const char* str){
    auto cp = Codepoint(StringPiece((const u8*)str,3));
    return Codepoint(StringPiece((const u8*)(str),3));
  }

  template <typename... T>
  static util::FlatMap<char32_t, Codepoint>  codemapsfor(T... vals){
    util::FlatMap<char32_t, Codepoint> result;
    std::initializer_list<std::pair<char32_t, const char*>> params = {P{vals} ...};
    for (auto &elem: params){
        result.insert(std::make_pair(elem.first, toCodepoint(elem.second )));
    }
    return result;
  }
  // Character Mappings
  const util::FlatMap<char32_t, Codepoint> lower2upper = 
  codemapsfor( P(u'ぁ', "あ"), P(u'ぃ', "い"),
               P(u'ぅ', "う"), P(u'ぇ', "え"),
               P(u'ぉ', "お"), P(u'ゎ', "わ"), 
               P(u'ヶ', "ケ"), P(u'ケ', "ヶ"));
    
  const util::FlatMap<char32_t, Codepoint> prolongedMap =   
  codemapsfor( P(u'か', "あ"), P(u'が', "あ"),P(u'ば', "あ"),P(u'ま', "あ"),
               P(u'ゃ', "あ"), 
               P(u'い', "い"), P(u'き', "い"), P(u'し', "い"), P(u'ち', "い"),
               P(u'に', "い"), P(u'ひ', "い"), P(u'じ', "い"), P(u'け', "い"),
               P(u'せ', "い"), P(u'へ', "い"), P(u'め', "い"), P(u'れ', "い"),
               P(u'げ', "い"), P(u'ぜ', "い"), P(u'で', "い"), P(u'べ', "い"),
               P(u'ぺ', "い"),
               P(u'く', "う"), P(u'す', "う"), P(u'つ', "う"), P(u'ふ', "う"),
               P(u'ゆ', "う"), P(u'ぐ', "う"), P(u'ず', "う"), P(u'ぷ', "う"),
               P(u'ゅ', "う"), P(u'お', "う"), P(u'こ', "う"), P(u'そ', "う"),
               P(u'と', "う"), P(u'の', "う"), P(u'ほ', "う"), P(u'も', "う"),
               P(u'よ', "う"), P(u'ろ', "う"), P(u'ご', "う"), P(u'ぞ', "う"), 
               P(u'ど', "う"), P(u'ぼ', "う"), P(u'ぽ', "う"), P(u'ょ', "う"),
               P(u'え', "え"),P(u'ね', "え"));

  const util::FlatSet<char32_t> lowerList{u'ぁ', u'ぃ', u'ぅ', u'ぇ', u'ぉ'};

  const util::FlatMap<char32_t, char32_t> lowerMap{
      {u'か', u'ぁ'}, {u'さ', u'ぁ'}, {u'た', u'ぁ'}, {u'な', u'ぁ'}, {u'は', u'ぁ'},
      {u'ま', u'ぁ'}, {u'や', u'ぁ'}, {u'ら', u'ぁ'}, {u'わ', u'ぁ'}, {u'が', u'ぁ'},
      {u'ざ', u'ぁ'}, {u'だ', u'ぁ'}, {u'ば', u'ぁ'}, {u'ぱ', u'ぁ'}, {u'い', u'ぃ'},
      {u'し', u'ぃ'}, {u'に', u'ぃ'}, {u'り', u'ぃ'}, {u'ぎ', u'ぃ'}, {u'じ', u'ぃ'},
      {u'ね', u'ぃ'}, {u'れ', u'ぃ'}, {u'ぜ', u'ぃ'}, {u'う', u'ぅ'}, {u'く', u'ぅ'},
      {u'す', u'ぅ'}, {u'ふ', u'ぅ'}, {u'む', u'ぅ'}, {u'る', u'ぅ'}, {u'よ', u'ぅ'},
      {u'け', u'ぇ'}, {u'せ', u'ぇ'}, {u'て', u'ぇ'}, {u'め', u'ぇ'}, {u'れ', u'ぇ'},
      {u'ぜ', u'ぇ'}, {u'で', u'ぇ'}, {u'こ', u'ぉ'}, {u'そ', u'ぉ'}, {u'の', u'ぉ'},
      {u'も', u'ぉ'}, {u'よ', u'ぉ'}, {u'ろ', u'ぉ'}, {u'ぞ', u'ぉ'}, {u'ど', u'ぉ'}};

  CharacterMap() noexcept {}
};

const CharacterMap &CMaps() {
  static CharacterMap singleton_;
  return singleton_;
}


inline bool CheckRemovableChoon(bool preIsDeleted, const std::vector<Codepoint>& codepoints, size_t pos){
  // 長音記号で, 直前が削除されたか、直前が平仮名、直前が漢字かつ直後が平仮名
  if (pos == 0) return false;
  bool isLastCharacter = !(pos + 1 < codepoints.size());
  bool isFirstCharacter = !(pos>0);
  const Codepoint& currentCp = codepoints[pos];
  CharacterClass preCharType = (isFirstCharacter)? CharacterClass::FAMILY_OTHERS: codepoints[pos-1].charClass;
  CharacterClass postCharType = (isLastCharacter)? CharacterClass::FAMILY_OTHERS: codepoints[pos+1].charClass;

  return ( ( preIsDeleted || 
             IsCompatibleCharClass(preCharType, CharacterClass::KATAKANA) ||
             IsCompatibleCharClass(preCharType, CharacterClass::HIRAGANA) ||
             (IsCompatibleCharClass(preCharType, CharacterClass::KANJI) &&
              (!isLastCharacter && IsCompatibleCharClass(postCharType, CharacterClass::HIRAGANA)))) &&
           ( currentCp.codepoint == DEF_PROLONG_SYMBOL1 ||
             currentCp.codepoint == DEF_PROLONG_SYMBOL2));
}

inline bool CheckRmovableHatsuon(bool preIsDeleted, const std::vector<Codepoint>& codepoints, size_t pos){
  // 直前が削除されていて、現在文字が"っ"、かつ、直後が文末もしくは記号の場合も削除
  if (pos == 0) return false;
  bool isLastCharacter = !(pos + 1 < codepoints.size());
  const Codepoint& currentCp = codepoints[pos];
  CharacterClass postCharType = (isLastCharacter)? CharacterClass::FAMILY_OTHERS: codepoints[pos+1].charClass;

  return (preIsDeleted && currentCp.codepoint == DEF_PROLONG_SYMBOL3 &&
                     (isLastCharacter || IsCompatibleCharClass(postCharType, CharacterClass::SYMBOL) ));
}

inline bool CheckRemovableYouon(bool preIsDeleted, const std::vector<Codepoint>& codepoints, size_t pos){
  /* 直前の文字が対応する平仮名の小書き文字、 
   * ("かぁ" > "か(ぁ)" :()内は削除の意味)
   * または、削除された同一の小書き文字の場合は削除
   * ("か(ぁ)ぁ" > "か(ぁ)(ぁ)") */
  if (pos == 0) return false;
  const Codepoint& currentCp = codepoints[pos];
  const Codepoint& lastCp = codepoints[pos-1];

  auto itr = CMaps().lowerMap.find(lastCp.codepoint);
  return (( itr != CMaps().lowerMap.end() &&
            itr->second == currentCp.codepoint) ||
          ( preIsDeleted &&
            CMaps().lowerList.find(currentCp.codepoint) != CMaps().lowerList.end() &&
            currentCp.codepoint == lastCp.codepoint));
}

inline bool CheckSubstituteChoon(const std::vector<Codepoint>& codepoints, size_t pos){
    // (ーの文字 || 〜の文字)  && ２文字目以降
    if (pos > 0 && (codepoints[pos].codepoint == DEF_PROLONG_SYMBOL1 ||
              codepoints[pos].codepoint == DEF_PROLONG_SYMBOL2 ) ){
        auto itr = CMaps().prolongedMap.find(codepoints[pos-1].codepoint);
        return (itr != CMaps().prolongedMap.end());
    }else
        return false;
}

inline bool CheckSubstituteLower(const std::vector<Codepoint>& codepoints, size_t pos){
    auto itr = CMaps().lower2upper.find(codepoints[pos].codepoint);
    return (itr != CMaps().lower2upper.end()); 
}

int CharLattice::Parse(const std::vector<Codepoint>& codepoints) { 
    size_t length = codepoints.size();
    int preIsDeleted = 0;
    int nextPreIsDeleted = 0;

    Codepoint skipped(StringPiece((const u8*)"", (size_t)0));
    nodeList.clear(); // Lattice 
    nodeList.resize(length);

    for (size_t pos = 0; pos < length; ++pos) {
        const Codepoint& currentCp = codepoints[pos];
        nodeList[pos].emplace_back(currentCp, OptCharLattice::OPT_NORMAL);
        nextPreIsDeleted = 0;

        /* Double Width Characters */
        if (currentCp.hasClass(CharacterClass::FAMILY_DOUBLE)) { 
            /* Substitution */
            if (CheckSubstituteChoon(codepoints, pos)) { 
                // Substitute choon to boin (ex. ねーさん > ねえさん)
                auto itr = CMaps().prolongedMap.find(codepoints[pos-1].codepoint);
                LOG_INFO() << "<substitute_choon_boin> " << currentCp.bytes << ">" << itr->second.bytes << "@" << pos << "\n";
                nodeList[pos].emplace_back( itr->second, OptCharLattice::OPT_PROLONG_REPLACED );
            } else if (CheckSubstituteLower(codepoints, pos)){ 
                // Substitute lower character to upper character (ex. ねぇさん > ねえさん)
                auto itr = CMaps().lower2upper.find(currentCp.codepoint);
                LOG_INFO() << "<substitute_small_large> " << currentCp.bytes << ">" << itr->second.bytes << "@" << pos << "\n";
                nodeList[pos].emplace_back( itr->second, OptCharLattice::OPT_NORMALIZE);
            }
            
            /* Deletion */
            if ( (CheckRemovableChoon(preIsDeleted, codepoints, pos)) ||
                CheckRmovableHatsuon(preIsDeleted, codepoints, pos)) {
                LOG_INFO() << "<del_prolong> " << currentCp.bytes << "@" << pos << "\n";
                nodeList[pos].emplace_back(skipped, OptCharLattice::OPT_PROLONG_DEL);
                nextPreIsDeleted = 1;
            } else if (CheckRemovableYouon(preIsDeleted, codepoints, pos)) {
                LOG_INFO() << "<del_youon> @" << pos << "\n";
                nodeList[pos].emplace_back(skipped, OptCharLattice::OPT_PROLONG_DEL);
                nextPreIsDeleted = 1;
            }
        }
        preIsDeleted = nextPreIsDeleted;
    }
    constructed = true;
    return 0;
}

std::vector<CharLattice::DaTrieResult> CharLattice::OneStep(int leftPosition, int rightPosition) {
    std::vector<CharLattice::DaTrieResult> result;  
    
    std::vector<CharNode> *leftCharNodeList;
    // Initialize leftCharNodeList
    if (leftPosition < 0) {
        leftCharNodeList = &(CharRootNodeList);
    } else {
        leftCharNodeList = &(nodeList[leftPosition]);
    }

    size_t currentDaNodeList;
    auto trav = entries.traversal(); // root node

    for (CharNode &leftCharNode : (*leftCharNodeList)) {
        // i: node_index
        for (size_t i = 0; i < leftCharNode.daNodePos.size(); ++i) { 
            // rightPosition: 次の文字位置
            std::vector<CharNode> &rightCharNodeList = nodeList[rightPosition];
            
            for (CharNode &rightCharNode : rightCharNodeList) {
                if (rightCharNode.cp.codepoint == 0) { /* Skipped node */
                    if (leftPosition >= 0) { 
                        // Do not move on the double array.
                        auto status = trav.step("", leftCharNode.daNodePos[i]);

                        /* Generate node if mached */
                        if (status == TraverseStatus::Ok) { 
                            auto dicEntries = trav.entries();
                            i32 ptr = 0;
                            while (dicEntries.readOnePtr(&ptr)) {
                              auto nodeType = (leftCharNode.nodeType[i] |
                                 (rightCharNode.type & (~(OptCharLattice::OPT_PROLONG_DEL))) |
                                 OptCharLattice::OPT_PROLONG_DEL_LAST);
                              EntryPtr eptr(ptr);
                              result.push_back(std::make_pair(eptr, nodeType));
                            }
                        }                         

                        // 次の探索開始位置に登録
                        rightCharNode.nodeType.push_back(
                            leftCharNode.nodeType[i] | rightCharNode.type |
                            OptCharLattice::OPT_PROLONG_DEL_LAST); 
                        rightCharNode.daNodePos.push_back( leftCharNode.daNodePos[i]);

                        if (MostDistantPosition < rightPosition)
                            MostDistantPosition = rightPosition;
                    }
                } else { /* Normal node */
                    if ( (rightCharNode.type & OptCharLattice::OPT_PROLONG_REPLACE) == OptCharLattice::OPT_INVALID ||
                        ( (rightCharNode.type & OptCharLattice::OPT_PROLONG_REPLACE) != OptCharLattice::OPT_INVALID &&
                         leftPosition >= 0)) { 
                        // 長音置換ノードは先頭以外である必要がある
                        currentDaNodeList = leftCharNode.daNodePos[i];
                        auto status = trav.step(rightCharNode.cp.bytes, currentDaNodeList);

                        if (status == TraverseStatus::Ok) { 
                            auto nodeType = leftCharNode.nodeType[i] | rightCharNode.type;
                            // Generate only normalized nodes
                            if( (nodeType & OptCharLattice::OPT_NORMALIZED) != OptCharLattice::OPT_INVALID){
                                auto dicEntries = trav.entries();
                                i32 ptr = 0;
                                while (dicEntries.readOnePtr(&ptr)) {
                                  EntryPtr eptr(ptr);
                                  result.push_back(std::make_pair(eptr, nodeType));
                                }
                            }
                        }

                        if (status == TraverseStatus::Ok ||
                            status == TraverseStatus::NoLeaf) { 
                            
                            // 開始位置位置として追加
                            rightCharNode.nodeType.push_back( leftCharNode.nodeType[i] | rightCharNode.type);
                            rightCharNode.daNodePos.push_back( currentDaNodeList );

                            if (MostDistantPosition < rightPosition)
                                MostDistantPosition = rightPosition;
                        }
                    }
                }
            }
        }
    }
    return result;
} 

std::vector<CharLattice::CLResult> CharLattice::Search(size_t position) { 
    std::vector<CharLattice::CLResult> result;
    if (!constructed) return result;

    /* initialization */
    CharLattice::MostDistantPosition = position - 1; 
    for (auto charNode = nodeList.begin() + position;
         charNode < nodeList.end(); charNode++) {
        for (auto &cnode : *charNode) {
            cnode.daNodePos.clear();
            cnode.nodeType.clear();
        }
    }

    auto tokensOneChar = OneStep(-1, position);
    for (auto &pair : tokensOneChar) { //一文字のノード
        auto type = pair.second;
        result.push_back(std::make_tuple(pair.first, type, position, position+1));
    }

    for (size_t i = position + 1; i < nodeList.size(); i++) { // 二文字以上
        if (MostDistantPosition < static_cast<int>(i) - 1)
            break; // position まで辿りつけた文字がない場合
        auto tokens = OneStep(i - 1, i);

        for (auto &pair : tokens) {
            auto type = pair.second;
            result.push_back(std::make_tuple(pair.first, type, position, i+1 ));
        }
    }
    return result;
} 
}
}
}
}

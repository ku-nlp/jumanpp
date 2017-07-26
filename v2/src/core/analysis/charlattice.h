
#ifndef CHARLATTICE_H
#define CHARLATTICE_H

#include <unordered_map>
#include <unordered_set>
#include "util/status.hpp"
#include "util/string_piece.h"
#include <util/flatset.h>
#include <util/flatmap.h>
#include "util/logging.hpp"

#include "core/darts.h"
#include "core/darts_trie.h" //wrapper
#include "core/analysis/lattice_builder.h"
#include "core/dic_entries.h"
#include "core/analysis/analysis_input.h"

/* 謎の定数 */
// TODO 廃止
#define BYTES4CHAR 3 /* UTF-8 (usually) */

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

/* このWeigthのときは形態素候補から除く */
const i32 STOP_MRPH_WEIGHT = 255; 

enum class OptCharLattice : u32 {
  OPT_INVALID = 0x00000000,
  OPT_NORMAL = 0x00000001,
  OPT_NORMALIZE = 0x00000002,
  OPT_DEVOICE = 0x00000004,
  OPT_PROLONG_DEL = 0x00000008,
  OPT_PROLONG_REPLACE = 0x00000010,
  OPT_PROLONG_DEL_LAST = 0x00000020,

  OPT_PROLONG_REPLACED = OPT_NORMALIZE | OPT_PROLONG_REPLACE,
  OPT_NORMALIZED = OPT_NORMALIZE | OPT_PROLONG_DEL | OPT_PROLONG_REPLACE | OPT_PROLONG_DEL_LAST
};

inline OptCharLattice operator|(OptCharLattice c1, OptCharLattice c2) noexcept {
  return static_cast<OptCharLattice>(static_cast<u32>(c1) |
                                     static_cast<u32>(c2));
}

inline OptCharLattice operator!(OptCharLattice c1) noexcept {
  return static_cast<OptCharLattice>(!static_cast<u32>(c1));
}

inline OptCharLattice operator~(OptCharLattice c1) noexcept {
  return static_cast<OptCharLattice>(~static_cast<u32>(c1));
}

inline OptCharLattice operator&(OptCharLattice c1, OptCharLattice c2) noexcept {
  return static_cast<OptCharLattice>(static_cast<u32>(c1) &
                                     static_cast<u32>(c2));
}

const i32 NORMALIZED_LENGTH = 8; /* 非正規表記の処理で考慮する最大形態素長 */

using Codepoint = jumanpp::chars::InputCodepoint;
using CharacterClass = jumanpp::chars::CharacterClass;

inline Codepoint toCodepoint(const char* str){
    auto cp = Codepoint(StringPiece((const u8*)str,3));
    LOG_INFO() << str << ":" << cp.bytes;
    return Codepoint(StringPiece((const u8*)(str),3));
}

class CharNode { //{{{
  public:
    Codepoint cp;
    OptCharLattice type = OptCharLattice::OPT_INVALID;

    // おなじ表層で異なるPOS が複数ある場合には，da_node_pos が複数になる．
    std::vector<size_t> da_node_pos; 
    std::vector<OptCharLattice> node_type;

  public:
    CharNode(const Codepoint &cp_in, const OptCharLattice init_type) noexcept:
        cp(cp_in){
        type = init_type;
    };
}; //}}}

class CharLattice { //{{{
  private:
    bool constructed = false;
    static bool initialized;
    const dic::DictionaryEntries& entries;

  public:
    typedef std::pair<EntryPtr, OptCharLattice> da_trie_result;
    typedef std::tuple<EntryPtr, OptCharLattice, LatticePosition, LatticePosition> CLResult;
    std::vector<std::vector<CharNode>> node_list;
    size_t CharNum;
    int MostDistantPosition;
    std::vector<size_t> char_byte_length;

  public:
    int parse(const std::vector<Codepoint>& codepoints); 
    std::vector<da_trie_result> da_search_one_step(int left_position, int right_position);

    std::vector<CharLattice::CLResult> da_search_from_position(size_t position); 

    CharLattice(const dic::DictionaryEntries& entries_) : 
        entries(entries_), 
        CharRootNodeList{CharNode(Codepoint(StringPiece((const u8*)"root", 4)), OptCharLattice::OPT_INVALID)} {
        // RootNode
        CharRootNodeList.back().da_node_pos.push_back(0);
        CharRootNodeList.back().node_type.push_back(OptCharLattice::OPT_INVALID); //初期値は0で良い？

        if (!initialized){
            initialized = true;

            for(auto& p:lower2upper_def){
                lower2upper.insert(std::make_pair(p.first, toCodepoint(p.second)));
            }
            for(auto& p:prolonged_map_def){
                prolonged_map.insert(std::make_pair(p.first, toCodepoint(p.second)));
            }
        }
    };

  public: // クラス定数;
    /* initialization for root node (starting node for looking up double array) */
    std::vector<CharNode> CharRootNodeList;
    const static util::FlatMap<char32_t, const char*> lower2upper_def;
    static util::FlatMap<char32_t, Codepoint> lower2upper;
    const static util::FlatMap<char32_t, const char*> prolonged_map_def;
    static util::FlatMap<char32_t, Codepoint> prolonged_map;


}; //}}}

} // charlattice
} // analysis
} // core
} // jumanpp

#endif

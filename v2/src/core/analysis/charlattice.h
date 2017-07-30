
#ifndef CHARLATTICE_H
#define CHARLATTICE_H

#include "util/status.hpp"
#include <util/flatset.h>
#include <util/flatmap.h>

#include "util/logging.hpp"
#include "core/analysis/lattice_builder.h"
#include "core/dic_entries.h"
#include "core/analysis/analysis_input.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

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

using Codepoint = jumanpp::chars::InputCodepoint;
using CharacterClass = jumanpp::chars::CharacterClass;

inline Codepoint toCodepoint(const char* str){
    auto cp = Codepoint(StringPiece((const u8*)str,3));
    return Codepoint(StringPiece((const u8*)(str),3));
}

class CharNode {
  public:
    Codepoint cp;
    OptCharLattice type = OptCharLattice::OPT_INVALID;

    std::vector<size_t> da_node_pos; 
    std::vector<OptCharLattice> node_type;

  public:
    CharNode(const Codepoint &cp_in, const OptCharLattice init_type) noexcept:
        cp(cp_in){
        type = init_type;
    };
}; 

class CharLattice { 
  private:
    bool constructed = false;
    static bool initialized;
    const dic::DictionaryEntries& entries;
    std::vector<std::vector<CharNode>> node_list;

  public:
    typedef std::pair<EntryPtr, OptCharLattice> da_trie_result;
    typedef std::tuple<EntryPtr, OptCharLattice, LatticePosition, LatticePosition> CLResult;
    int MostDistantPosition;

    int parse(const std::vector<Codepoint>& codepoints); 
    std::vector<da_trie_result> da_search_one_step(int left_position, int right_position);
    std::vector<CharLattice::CLResult> da_search_from_position(size_t position); 

    CharLattice(const dic::DictionaryEntries& entries_) : 
        entries(entries_), 
        CharRootNodeList{CharNode(Codepoint(StringPiece((const u8*)"<root>", 6)), OptCharLattice::OPT_INVALID)} {
        // RootNode
        CharRootNodeList.back().da_node_pos.push_back(0);
        CharRootNodeList.back().node_type.push_back(OptCharLattice::OPT_INVALID); 

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

  public: 
    std::vector<CharNode> CharRootNodeList;
    const static util::FlatMap<char32_t, const char*> lower2upper_def;
    static util::FlatMap<char32_t, Codepoint> lower2upper;
    const static util::FlatMap<char32_t, const char*> prolonged_map_def;
    static util::FlatMap<char32_t, Codepoint> prolonged_map;
}; 

} // charlattice
} // analysis
} // core
} // jumanpp

#endif

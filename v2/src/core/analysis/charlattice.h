
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

    std::vector<size_t> daNodePos; 
    std::vector<OptCharLattice> nodeType;

  public:
    CharNode(const Codepoint &cpIn, const OptCharLattice initType) noexcept:
        cp(cpIn){
        type = initType;
    };
}; 

class CharLattice { 
  private:
    bool constructed = false;
//    static bool initialized;
    const dic::DictionaryEntries& entries;
    std::vector<std::vector<CharNode>> nodeList;

  public:
    typedef std::pair<EntryPtr, OptCharLattice> DaTrieResult;
    typedef std::tuple<EntryPtr, OptCharLattice, LatticePosition, LatticePosition> CLResult;
    int MostDistantPosition;

    int Parse(const std::vector<Codepoint>& codepoints); 
    std::vector<DaTrieResult> OneStep(int left_position, int right_position);
    std::vector<CharLattice::CLResult> Search(size_t position); 

    CharLattice(const dic::DictionaryEntries& entries_) : 
        entries(entries_), 
        CharRootNodeList{CharNode(Codepoint(StringPiece((const u8*)"<root>", 6)), OptCharLattice::OPT_INVALID)} {
        // RootNode
        CharRootNodeList.back().daNodePos.push_back(0);
        CharRootNodeList.back().nodeType.push_back(OptCharLattice::OPT_INVALID); 
    };

  public: 
    std::vector<CharNode> CharRootNodeList;
}; 

} // charlattice
} // analysis
} // core
} // jumanpp

#endif

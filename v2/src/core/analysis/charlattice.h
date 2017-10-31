
#ifndef CHARLATTICE_H
#define CHARLATTICE_H

#include <util/flatmap.h>
#include <util/flatset.h>
#include "util/status.hpp"

#include "core/analysis/analysis_input.h"
#include "core/analysis/lattice_builder.h"
#include "core/dic/dic_entries.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

template <typename T>
using Vec = util::memory::ManagedVector<T>;

enum class Modifiers : u32 {
  EMPTY = 0x00000000,
  ORIGINAL = 0x00000001,
  REPLACE_SMALLKANA = 0x00000002,
  REPLACE = 0x00000004,
  DELETE = 0x00000008,
  REPLACE_PROLONG = 0x00000010,
  DELETE_LAST = 0x00000020,
  DELETE_PROLONG = 0x00000040,
  DELETE_HASTSUON = 0x0000080,
  DELETE_SMALLKANA = 0x00000100,
  REPLACE_EROW_WITH_E = 0x00000200,
};

inline Modifiers operator|(Modifiers c1, Modifiers c2) noexcept {
  return static_cast<Modifiers>(static_cast<u32>(c1) | static_cast<u32>(c2));
}

inline Modifiers operator!(Modifiers c1) noexcept {
  return static_cast<Modifiers>(!static_cast<u32>(c1));
}

inline Modifiers operator~(Modifiers c1) noexcept {
  return static_cast<Modifiers>(~static_cast<u32>(c1));
}

inline Modifiers operator&(Modifiers c1, Modifiers c2) noexcept {
  return static_cast<Modifiers>(static_cast<u32>(c1) & static_cast<u32>(c2));
}

using Codepoint = jumanpp::chars::InputCodepoint;
using CharacterClass = jumanpp::chars::CharacterClass;

inline bool ExistFlag(Modifiers general, Modifiers query) {
  return (general & query) != Modifiers::EMPTY;
}

struct CharNode {
  Codepoint cp;
  Modifiers type = Modifiers::EMPTY;

  CharNode(const Codepoint& cpIn, const Modifiers initType) noexcept
      : cp(cpIn) {
    type = initType;
  };

  bool hasType(Modifiers type) const noexcept {
    return (this->type & type) != Modifiers::EMPTY;
  }

  bool isReplace() const { return hasType(Modifiers::REPLACE_PROLONG); }

  bool isDelete() const { return hasType(Modifiers::DELETE); }
};

struct DaTrieResult {
  EntryPtr entryPtr;
  Modifiers flags;
};

struct CLResult {
  EntryPtr entryPtr;
  Modifiers flags;
  LatticePosition start;
  LatticePosition end;
};

class CharLattceTraversal;

class CharLattice {
 private:
  bool constructed = false;
  //    static bool initialized;
  const dic::DictionaryEntries& entries;
  Vec<Vec<CharNode>> nodeList;
  util::memory::PoolAlloc* alloc_;
  u32 notNormal = 0;

  void add(size_t pos, const Codepoint& cp, Modifiers flags) {
    if (flags != Modifiers::ORIGINAL) {
      notNormal += 1;
    }
    nodeList[pos].emplace_back(cp, flags);
  }

 public:
  int Parse(const std::vector<Codepoint>& codepoints);
  // Vec<DaTrieResult> oneStep(int left_position, int right_position);
  // Vec<CLResult> search(i32 position);

  CharLattice(const dic::DictionaryEntries& entries_,
              util::memory::PoolAlloc* alloc)
      : entries{entries_}, alloc_{alloc}, nodeList{alloc} {}

  bool isApplicable() const { return notNormal != 0; }

  friend class CharLattceTraversal;

  CharLattceTraversal traversal(util::ArraySlice<Codepoint> input);
};

struct TraverasalState {
  dic::DoubleArrayTraversal traversal;
  LatticePosition start;
  LatticePosition end;
  Modifiers allFlags;
  dic::TraverseStatus lastStatus;

  TraverasalState(const dic::DoubleArrayTraversal& t, LatticePosition start,
                  LatticePosition end, Modifiers flags)
      : traversal{t}, start{start}, end{end}, allFlags{flags} {}
};

class CharLattceTraversal {
  CharLattice& lattice_;
  util::ArraySlice<Codepoint> input_;
  Vec<TraverasalState*> states1_;
  Vec<TraverasalState*> states2_;
  Vec<TraverasalState*> buffer_;

  Vec<CLResult> result_;

 public:
  CharLattceTraversal(CharLattice& lattice, util::ArraySlice<Codepoint> input)
      : lattice_{lattice},
        input_{input},
        states1_{lattice.alloc_},
        states2_{lattice.alloc_},
        buffer_{lattice.alloc_},
        result_{lattice.alloc_} {}

  const Vec<CLResult>& candidates() const { return result_; }

  TraverasalState* make(const dic::DoubleArrayTraversal& t,
                        LatticePosition start, LatticePosition end,
                        Modifiers flags);

  void tryWalk(TraverasalState* pState, const Codepoint& codepoint,
               Modifiers newFlag, bool doStep);

  void doTraverseStep(i32 pos);

  bool lookupCandidatesFrom(i32 start);
};

}  // namespace charlattice
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif

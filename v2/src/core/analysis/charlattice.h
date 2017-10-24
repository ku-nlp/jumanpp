
#ifndef CHARLATTICE_H
#define CHARLATTICE_H

#include <util/flatmap.h>
#include <util/flatset.h>
#include "util/status.hpp"

#include "core/analysis/analysis_input.h"
#include "core/analysis/lattice_builder.h"
#include "core/dic_entries.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

template <typename T>
using Vec = util::memory::ManagedVector<T>;

enum class OptCharLattice : u32 {
  OPT_EMPTY = 0x00000000,
  OPT_ORIGINAL = 0x00000001,
  OPT_SMALL_TO_LARGE = 0x00000002,
  OPT_DEVOICE = 0x00000004,
  OPT_DELETE = 0x00000008,
  OPT_PROLONG_REPLACE = 0x00000010,
  OPT_PROLONG_DEL_LAST = 0x00000020,
  OPT_DELETE_PROLONG = 0x00000040,
  OPT_DELETE_HASTSUON = 0x0000080,
  OPT_DELETE_SMALLKANA = 0x00000100,
  OPT_PROLONG_EROW_WITH_E = 0x00000200,
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

inline bool ExistFlag(OptCharLattice general, OptCharLattice query) {
  return (general & query) != OptCharLattice::OPT_EMPTY;
}

struct CharNode {
  Codepoint cp;
  OptCharLattice type = OptCharLattice::OPT_EMPTY;

  CharNode(const Codepoint& cpIn, const OptCharLattice initType) noexcept
      : cp(cpIn) {
    type = initType;
  };

  bool hasType(OptCharLattice type) const noexcept {
    return (this->type & type) != OptCharLattice::OPT_EMPTY;
  }

  bool isReplace() const {
    return hasType(OptCharLattice::OPT_PROLONG_REPLACE);
  }

  bool isDelete() const { return hasType(OptCharLattice::OPT_DELETE); }
};

struct DaTrieResult {
  EntryPtr entryPtr;
  OptCharLattice flags;
};

struct CLResult {
  EntryPtr entryPtr;
  OptCharLattice flags;
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
  util::memory::ManagedAllocatorCore* alloc_;
  u32 notNormal = 0;

  void add(size_t pos, const Codepoint& cp, OptCharLattice flags) {
    if (flags != OptCharLattice::OPT_ORIGINAL) {
      notNormal += 1;
    }
    nodeList[pos].emplace_back(cp, flags);
  }

 public:
  int Parse(const std::vector<Codepoint>& codepoints);
  // Vec<DaTrieResult> oneStep(int left_position, int right_position);
  // Vec<CLResult> search(i32 position);

  CharLattice(const dic::DictionaryEntries& entries_,
              util::memory::ManagedAllocatorCore* alloc)
      : entries{entries_}, alloc_{alloc}, nodeList{alloc} {}

  bool isApplicable() const { return notNormal != 0; }

  friend class CharLattceTraversal;

  CharLattceTraversal traversal(util::ArraySlice<Codepoint> input);
};

struct TraverasalState {
  DoubleArrayTraversal traversal;
  LatticePosition start;
  LatticePosition end;
  OptCharLattice allFlags;
  TraverseStatus lastStatus;

  TraverasalState(const DoubleArrayTraversal& t, LatticePosition start,
                  LatticePosition end, OptCharLattice flags)
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

  TraverasalState* make(const DoubleArrayTraversal& t, LatticePosition start,
                        LatticePosition end, OptCharLattice flags);

  void tryWalk(TraverasalState* pState, const Codepoint& codepoint,
               OptCharLattice newFlag, bool doStep);

  void doTraverseStep(i32 pos);

  bool lookupCandidatesFrom(i32 start);
};

}  // namespace charlattice
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif

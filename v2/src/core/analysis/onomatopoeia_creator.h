//
// Created by nakao on 2017/05/24.
//

#ifndef JUMANPP_ONOMATOPOEIA_CREATOR_H
#define JUMANPP_ONOMATOPOEIA_CREATOR_H

#include "unk_nodes_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

enum class OnomatopoeicPattern : u16;

inline OnomatopoeicPattern operator&(const OnomatopoeicPattern& l,
                                     const OnomatopoeicPattern& r) {
  return static_cast<OnomatopoeicPattern>(
      static_cast<u16>(l) & static_cast<u16>(r)
  );
}

inline OnomatopoeicPattern operator|(const OnomatopoeicPattern& l,
                                     const OnomatopoeicPattern& r) {
  return static_cast<OnomatopoeicPattern>(
      static_cast<u16>(l) | static_cast<u16>(r)
  );
}

enum class OnomatopoeicPattern : u16 {
  None = 0,
  ABAB = 1 << 2, ABCABC = 1 << 3, ABCDABCD = 1 << 4,
  All = ABAB | ABCABC | ABCDABCD
};

class OnomatopoeiaUnkMaker : public UnkMaker {
  dic::DictionaryEntries entries_;
  chars::CharacterClass charClass_;
  UnkNodeConfig info_;

  using Pattern = OnomatopoeicPattern;
  using CodepointStorage = std::vector<jumanpp::chars::InputCodepoint>;

  static const size_t MaxOnomatopoeiaLength = 8;

 public:
  OnomatopoeiaUnkMaker(const dic::DictionaryEntries& entries_,
                       chars::CharacterClass charClass_, UnkNodeConfig&& info_);

  bool spawnNodes(const AnalysisInput& input, UnkNodesContext* ctx,
                  LatticeBuilder* lattice) const override;

  Pattern FindOnomatopoeia(const CodepointStorage & codepoints,
                           LatticePosition start) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif //JUMANPP_ONOMATOPOEIA_CREATOR_H

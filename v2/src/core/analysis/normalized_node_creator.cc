//
// Created by Hajime Morita on 2017/07/17.
//

#include "normalized_node_creator.h"
#include "core_config.h"

namespace jumanpp {
namespace core {
namespace analysis {

using Codepoint = jumanpp::chars::InputCodepoint;
using CharacterClass = jumanpp::chars::CharacterClass;

bool NormalizedNodeMaker::spawnNodes(const AnalysisInput& input,
                                     UnkNodesContext* ctx,
                                     LatticeBuilder* lattice) const {
  auto& points = input.codepoints();
  charlattice::CharLattice cl{entries_, ctx->alloc()};
  cl.Parse(points);

  if (!cl.isApplicable()) {
    return true;
  }

  // number of input characters
  auto totalPoints = (LatticePosition)points.size();
  auto trav = cl.traversal(points);

  // for any position of input string
  for (LatticePosition begin = 0; begin < totalPoints; ++begin) {
    // traverse (access all of the nodes)
    if (trav.lookupCandidatesFrom(begin)) {
      for (auto& r : trav.candidates()) {
        auto ptr = r.entryPtr;
        auto type = r.flags;
        auto begin = r.start;
        auto end = r.end;
        auto entryData = entries_.entryAtPtr(ptr);
        u32 buffer[JPP_MAX_DIC_FIELDS];
        auto nfields = entryData.fill(buffer, JPP_MAX_DIC_FIELDS);
        util::ArraySlice<u32> fieldData{buffer, nfields};
        auto fieldCode = static_cast<i32>(type);
        auto surface_ptr = ctx->makePtr(input.surface(begin, end), info_,
                                        fieldData, fieldCode);
        lattice->appendSeed(surface_ptr, begin, end);
      }
    }
  }

  return true;
}

NormalizedNodeMaker::NormalizedNodeMaker(const dic::DictionaryEntries& entries_,
                                         UnkNodeConfig&& info_)
    : entries_(entries_), info_(info_) {}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

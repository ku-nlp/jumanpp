//
//
// Created by Hajime Morita on 2017/07/17.
//

#include "normalized_node_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

using Codepoint = jumanpp::chars::InputCodepoint;
using CharacterClass = jumanpp::chars::CharacterClass;


bool NormalizedNodeMaker::spawnNodes(const AnalysisInput& input, UnkNodesContext *ctx, 
                                       LatticeBuilder* lattice) const {
  LOG_INFO() << "spawnNodes" << "\n";

  CodepointStorage points = input.codepoints();
  charlattice::CharLattice cl(entries_);
  
  LOG_INFO() << "call CharLattice::parse" << "\n";
  cl.parse(points);
  
  // number of input characters
  LatticePosition totalPoints = (LatticePosition)points.size();

  // for any position of input string
  for (LatticePosition begin = 0; begin < totalPoints; ++begin) {
    // traverse (access all of the nodes)
    auto result = cl.da_search_from_position(begin); 
    for(charlattice::CharLattice::CLResult& r: result){
        auto ptr = std::get<0>(r);
        auto type = std::get<1>(r);
        auto begin = std::get<2>(r);
        auto end = std::get<3>(r);
        auto surface_ptr = ctx->makePtr(input.surface(begin, end), info_, false);
        lattice->appendSeed(surface_ptr, begin, end);
    }
  }

  return true;
}

NormalizedNodeMaker::NormalizedNodeMaker(
    const dic::DictionaryEntries& entries_, 
    UnkNodeConfig&& info_)
    : entries_(entries_), info_(info_) {
    LOG_INFO() << "NormalizeNodeMaker constructer" << "\n";
    }

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

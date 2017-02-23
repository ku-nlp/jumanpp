//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_DICTIONARY_NODE_CREATOR_H
#define JUMANPP_DICTIONARY_NODE_CREATOR_H

#include "core/analysis/analysis_input.h"
#include "core/analysis/lattice_builder.h"
#include "core/dic_entries.h"

namespace jumanpp {
namespace core {
namespace analysis {

class DictionaryNodeCreator {
  dic::DictionaryEntries entries_;

 public:
  DictionaryNodeCreator(const dic::DictionaryEntries& entries_);

  bool spawnNodes(const AnalysisInput& input, LatticeBuilder* lattice);
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_DICTIONARY_NODE_CREATOR_H

//
// Created by Arseny Tolmachev on 2017/03/05.
//

#include "unk_nodes.h"
#include "core/core.h"
#include "dic_reader.h"
#include "normalized_node_creator.h"
#include "numeric_creator.h"
#include "onomatopoeia_creator.h"
#include "unk_nodes_creator.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

class UnkMakerFactory {
  DicReader rdr;
  dic::DictionaryEntries entries;
  const spec::AnalysisSpec& spec_;

 public:
  UnkMakerFactory(const CoreHolder& core)
      : rdr{core.dic()}, entries{core.dic().entries()}, spec_{core.spec()} {}

  Status handlePrefixIndex(const spec::UnkProcessorDescriptor& info,
                           UnkNodeConfig* pConfig) const {
    if (!info.features.empty()) {
      pConfig->targetPlaceholder = info.features[0].targetPlaceholder;
      if (info.features.size() != 1) {
        return JPPS_NOT_IMPLEMENTED
               << "we support only one placeholder per unk";
      }
    }
    return Status::Ok();
  }

  Status make(UnkMakers* result) const {
    for (auto& x : spec_.unkCreators) {
      // resolve unk processing stage
      std::vector<std::unique_ptr<UnkMaker>>* stage;
      switch (x.priority) {
        case 0: {
          stage = &result->stage1;
          break;
        }
        case 1: {
          stage = &result->stage2;
          break;
        }
        default:
          return Status::InvalidState() << "UNK " << x.name << ": priority "
                                        << x.priority << " is not supported";
      }

      auto eptr = EntryPtr{x.patternPtr};
      UnkNodeConfig cfg{rdr.readEntry(eptr), eptr};
      util::copy_insert(x.replaceFields, cfg.replaceWithSurface);
      handlePrefixIndex(x, &cfg);
      cfg.fillPatternFields();

      // resolve processor itself
      switch (x.type) {
        case spec::UnkMakerType::Chunking: {
          stage->emplace_back(
              new ChunkingUnkMaker{entries, x.charClass, std::move(cfg)});
          break;
        }
        case spec::UnkMakerType::Single: {
          stage->emplace_back(
              new SingleUnkMaker{entries, x.charClass, std::move(cfg)});
          break;
        }
        case spec::UnkMakerType::Onomatopoeia: {
          stage->emplace_back(
              new OnomatopoeiaUnkMaker{entries, x.charClass, std::move(cfg)});
          break;
        }
        case spec::UnkMakerType::Numeric: {
          stage->emplace_back(
              new NumericUnkMaker{entries, x.charClass, std::move(cfg)});
          break;
        }
        case spec::UnkMakerType::Normalize: {
          stage->emplace_back(new NormalizedNodeMaker{entries, std::move(cfg)});
          break;
        }
        default:
          return Status::NotImplemented()
                 << x.name << ": unk is not implemented";
      }
    }
    return Status::Ok();
  }
};

Status makeMakers(const CoreHolder& core, UnkMakers* result) {
  UnkMakerFactory factory{core};
  return factory.make(result);
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

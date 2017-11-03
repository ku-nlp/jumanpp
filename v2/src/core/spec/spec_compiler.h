//
// Created by Arseny Tolmachev on 2017/11/02.
//

#ifndef JUMANPP_SPEC_COMPILER_H
#define JUMANPP_SPEC_COMPILER_H

#include <unordered_map>
#include "core/impl/int_seq_util.h"
#include "spec_dsl.h"
#include "util/flatmap.h"

namespace jumanpp {
namespace core {
namespace spec {

namespace dsl {
class FeatureBuilder;
}

struct PatternFeatureInfo {
  i32 number = -1;
  u32 usage = 0;
};

struct AccessFeatureInfo {
  i32 number = -1;
  i32 dicFinalNumber = -1;
  i32 shift = -1;
  std::vector<i32> trueBranch;
  std::vector<i32> falseBranch;
};

class SpecCompiler {
  const dsl::ModelSpecBuilder& bldr_;
  AnalysisSpec* result_;
  util::FlatMap<StringPiece, AccessFeatureInfo> computeByName_;
  util::FlatMap<impl::DicEntryFeatures, PatternFeatureInfo,
                impl::DicEntryFeaturesHash>
      patternByCombo_;

  i32 currentOffset_ = 32;
  i32 currentFlagField_ = InvalidInt;

  FieldDescriptor* fieldByName(StringPiece name, bool allowNull = false);
  dsl::FeatureBuilder* findFeature(StringPiece name);
  AccessFeatureInfo& registerFeature(StringPiece piece);

 public:
  SpecCompiler(const dsl::ModelSpecBuilder& bldr, AnalysisSpec* result)
      : bldr_{bldr}, result_{result} {}

  Status buildFields();
  Status buildFeatures();
  Status buildUnkProcessors();
  Status createTrainSpec();
  Status build();

  void gatherFeatureInfo();
  void groupPatternFeatures();
  void fillNgramFeatures();
  Status makeDicFeatures();
  Status fillRemainingFeatures();
  i32 addDicImport(const FieldDescriptor& descriptor, DicImportKind kind);
  Status makeMatchValueFeature(dsl::FeatureBuilder* f);
  Status makeMatchCsvFeature(dsl::FeatureBuilder* f);
  std::pair<i32, i32> makeIntSlot(int width);
  DicImportDescriptor& complexImportCommonInit(const dsl::FeatureBuilder* f);

  Status copyPrimitiveInfo(PrimitiveFeatureDescriptor* desc,
                           const dsl::FeatureBuilder& fBldr,
                           const AccessFeatureInfo& afi);

  void buildAliasingSet();
};

}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_COMPILER_H

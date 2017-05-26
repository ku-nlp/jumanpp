//
// Created by Arseny Tolmachev on 2017/05/26.
//

#ifndef JUMANPP_FEATURE_CODEGEN_H
#define JUMANPP_FEATURE_CODEGEN_H

#include "util/codegen.h"
#include "core/features_api.h"

namespace jumanpp {
namespace core {
namespace features {
namespace codegen {

struct FeatureCodegenConfig {
  std::string filename;
  std::string className;
  std::string baseDirectory;
};

class StaticFeatureCodegen {
  FeatureCodegenConfig config_;
public:
  explicit StaticFeatureCodegen(const FeatureCodegenConfig &config);
  Status writeSource(const std::string& filename, const FeatureHolder& features);
  Status writeHeader(const std::string& filename);
  Status generateAndWrite(const FeatureHolder& features);
};

} // codegen
} // features
} // core
} // jumanpp

#endif //JUMANPP_FEATURE_CODEGEN_H

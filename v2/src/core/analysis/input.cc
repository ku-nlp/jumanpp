///
// Created by Arseny Tolmachev on 2017/02/23.
//

#include <string>
#include "util/characters.hpp"
#include "util/status.hpp"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

class AnalysisInput {
  size_t max_size_;
  std::string raw_input;
  std::vector<jumanpp::chars::InputCodepoint> codepoints;

 public:
  AnalysisInput(size_t maxSize = 8 * 1024) : max_size_{maxSize} {
    raw_input.reserve(maxSize);
  }

  Status reset(StringPiece data) {
    if (data.size() > max_size_) {
      return Status::InvalidParameter()
             << "byte size of input string (" << data.size()
             << ") is greater than maximum allowed (" << max_size_ << ")";
    }

    raw_input.clear();
    codepoints.clear();
    raw_input.append(data.begin(), data.end());
    return chars::preprocessRawData(raw_input, &codepoints);
  }
};

struct LatticeNodeSeed {
  i32 entryPtr;
  i16 codepointStart;
  i16 codepointEnd;
};

class LatticeBuilder {
public:
  void appendSeed(LatticeNodeSeed seed) {}
};

}  // analysis
}  // core
}  // jumanpp
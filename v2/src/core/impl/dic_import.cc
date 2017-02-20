//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "dic_import.h"
#include <algorithm>

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

Status StringFieldImporter::makeStorage(std::ostream& result) {
  std::vector<std::pair<StringPiece, i32>> forSort;
  for (auto& obj : mapping_) {
    forSort.emplace_back(obj.first, obj.second);
  }

  std::sort(forSort.begin(), forSort.end(),
            [](auto& a, auto& b) { return a.second > b.second; });
  util::CodedBuffer buf;

  i32 buffer_position = 0;
  for (const auto& obj : forSort) {
    buf.writeString(obj.first);
    auto data = buf.contents();
    mapping_[obj.first] = buffer_position;
    buffer_position += static_cast<i32>(data.size());
    result << data;
    buf.reset();
  }

  return Status::Ok();
}

}  // impl
}  // dic
}  // core
}  // jumanpp
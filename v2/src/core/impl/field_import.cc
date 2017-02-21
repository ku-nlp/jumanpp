//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "field_import.h"
#include <algorithm>

namespace jumanpp {
namespace core {
namespace dic {
namespace impl {

Status StringFieldImporter::makeStorage(util::CodedBuffer* result) {
  std::vector<std::pair<StringPiece, i32>> forSort;
  for (auto& obj : mapping_) {
    forSort.emplace_back(obj.first, obj.second);
  }

  std::sort(forSort.begin(), forSort.end(),
            [](auto& a, auto& b) { return a.second > b.second; });

  util::CodedBuffer& buf = *result;

  for (const auto& obj : forSort) {
    auto ptr = static_cast<i32>(buf.position());
    mapping_[obj.first] = ptr;
    buf.writeString(obj.first);
  }

  return Status::Ok();
}

i32 StringListFieldImporter::fieldPointer(const util::CsvReader& csv) {
  auto sp = csv.field(field_);

  values_.clear();

  while (sp.size() > 0) {
    auto space = std::find(sp.begin(), sp.end(), ' ');
    auto key = StringPiece{sp.begin(), space};
    values_.push_back(mapping_[key]);
    if (space == sp.end()) {
      break;
    }
    sp = StringPiece{space + 1, sp.end()};
  }

  i32 result = static_cast<i32>(buffer_->position());
  writePtrsAsDeltas(values_, *buffer_);
  return result;
}

bool StringListFieldImporter::importFieldValue(const util::CsvReader& csv) {
  auto sp = csv.field(field_);
  while (sp.size() > 0) {
    auto space = std::find(sp.begin(), sp.end(), ' ');
    auto key = StringPiece{sp.begin(), space};
    mapping_[key] += 1;
    if (space == sp.end()) {
      return true;
    }
    sp = StringPiece{space + 1, sp.end()};
  }

  return true;
}
}  // impl
}  // dic
}  // core
}  // jumanpp
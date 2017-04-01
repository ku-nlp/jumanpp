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
  return storage_->makeStorage(result);
}

i32 StringListFieldImporter::fieldPointer(const util::CsvReader& csv) {
  JPP_DCHECK_NE(buffer_, nullptr);
  auto sp = csv.field(field_);

  // check if the field is ignored
  if (ignore_ == sp || sp.size() == 0) {
    return 0;
  }

  values_.clear();

  while (sp.size() > 0) {
    auto space = std::find(sp.begin(), sp.end(), ' ');
    auto key = StringPiece{sp.begin(), space};
    auto v = storage_->valueOf(key);
    JPP_DCHECK_NE(v, -1);
    values_.push_back(v);
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
  if (ignore_ == sp) {
    return true;
  }
  while (sp.size() > 0) {
    auto space = std::find(sp.begin(), sp.end(), ' ');
    auto key = StringPiece{sp.begin(), space};
    storage_->increaseFieldValueCount(key);
    if (space == sp.end()) {
      return true;
    }
    sp = StringPiece{space + 1, sp.end()};
  }

  return true;
}

void StringListFieldImporter::injectFieldBuffer(util::CodedBuffer* buffer) {
  buffer_ = buffer;
  // put zero (zero length sequence) at the beginning
  buffer->writeVarint(0);
}

IntFieldImporter::IntFieldImporter(i32 field)
    : fld_{field}, re_{"^\\d+$", std::regex::optimize | std::regex::basic} {}

i32 IntFieldImporter::fieldPointer(const util::CsvReader& csv) {
  StringPiece sp = csv.field(fld_);
  if (sp.size() == 0) {
    return 0;
  }
  auto end = sp.char_end();
  long res = std::strtol(sp.char_begin(), const_cast<char**>(&end), 10);
  return static_cast<i32>(res);
}

bool IntFieldImporter::importString(StringPiece sp) {
  if (sp.size() == 0) return true;
  return std::regex_match(sp.char_begin(), sp.char_end(), mr_, re_);
}

Status StringStorage::makeStorage(util::CodedBuffer* result) {
  std::vector<std::pair<StringPiece, i32>> forSort;
  for (auto& obj : mapping_) {
    forSort.emplace_back(obj.first, obj.second);
  }

  std::sort(forSort.begin(), forSort.end(),
            [](auto& a, auto& b) { return a.second > b.second; });

  util::CodedBuffer& buf = *result;

  // always put empty string at 0
  buf.writeString("");

  for (const auto& obj : forSort) {
    auto ptr = static_cast<i32>(buf.position());
    mapping_[obj.first] = ptr;
    buf.writeString(obj.first);
  }

  return Status::Ok();
}
}  // namespace impl
}  // namespace dic
}  // namespace core
}  // namespace jumanpp
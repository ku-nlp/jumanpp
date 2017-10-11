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

Status StringListFieldImporter::makeStorage(util::CodedBuffer* result) {
  buffer_->writeString("");
  return StringFieldImporter::makeStorage(result);
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

void StringKeyValueListFieldImporter::injectFieldBuffer(
    util::CodedBuffer* buffer) {
  buffer_ = buffer;
}

bool StringKeyValueListFieldImporter::importFieldValue(
    const util::CsvReader& csv) {
  auto sp = csv.field(field_);
  if (ignore_ == sp) {
    return true;
  }
  auto sepSize = entrySeparator_.size();
  auto kvsepSize = kvSeparator_.size();
  while (sp.size() > 0) {
    auto space = std::search(sp.begin(), sp.end(), entrySeparator_.begin(),
                             entrySeparator_.end());
    auto entry = StringPiece{sp.begin(), space};
    auto sepIter = std::search(sp.begin(), space, kvSeparator_.begin(),
                               kvSeparator_.end());
    if (sepIter == space) {
      storage_->increaseFieldValueCount(entry);
    } else {
      auto key = StringPiece{sp.begin(), sepIter};
      auto value = StringPiece{sepIter + kvsepSize, space};
      storage_->increaseFieldValueCount(key);
      storage_->increaseFieldValueCount(value);
    }
    if (space == sp.end()) {
      return true;
    }
    sp = StringPiece{space + sepSize, sp.end()};
  }

  return true;
}

i32 StringKeyValueListFieldImporter::fieldPointer(const util::CsvReader& csv) {
  JPP_DCHECK_NE(buffer_, nullptr);
  auto sp = csv.field(field_);

  // check if the field is ignored
  if (ignore_ == sp || sp.size() == 0) {
    return 0;
  }

  values_.clear();
  auto sepSize = entrySeparator_.size();
  auto kvsepSize = kvSeparator_.size();

  while (sp.size() > 0) {
    auto space = std::search(sp.begin(), sp.end(), entrySeparator_.begin(),
                             entrySeparator_.end());
    auto sepIter = std::search(sp.begin(), space, kvSeparator_.begin(),
                               kvSeparator_.end());

    if (sepIter == space) {
      auto entry = StringPiece{sp.begin(), space};
      auto keyIdx = storage_->valueOf(entry);
      JPP_DCHECK_NE(keyIdx, -1);
      values_.emplace_back(keyIdx, -1);
    } else {
      auto key = StringPiece{sp.begin(), sepIter};
      auto value = StringPiece{sepIter + kvsepSize, space};
      auto keyIdx = storage_->valueOf(key);
      auto valueIdx = storage_->valueOf(value);
      JPP_DCHECK_NE(keyIdx, -1);
      JPP_DCHECK_NE(valueIdx, -1);
      values_.emplace_back(keyIdx, valueIdx);
    }
    if (space == sp.end()) {
      break;
    }
    sp = StringPiece{space + sepSize, sp.end()};
  }

  std::sort(values_.begin(), values_.end(),
            [](const std::pair<i32, i32>& p1, const std::pair<i32, i32>& p2) {
              return p1.first < p2.first;
            });

  JPP_DCHECK_GT(values_.size(), 0);
  auto first = values_[0];
  i32 lastKey = first.first;

  local_.reset();

  local_.writeVarint(static_cast<u64>(values_.size()));

  u32 keyFlag = first.second == -1 ? 0 : 1;
  local_.writeVarint((static_cast<u64>(first.first) << 1) | keyFlag);
  if (keyFlag == 1) {
    local_.writeVarint(static_cast<u64>(first.second));
  }

  for (int i = 1; i < values_.size(); ++i) {
    auto pair = values_[i];
    keyFlag = pair.second == -1 ? 0 : 1;
    u64 keyDiff = static_cast<u64>(pair.first - lastKey);
    lastKey = pair.first;
    local_.writeVarint((keyDiff << 1) | keyFlag);
    if (keyFlag != 0) {
      local_.writeVarint(static_cast<u64>(pair.second));
    }
  }

  i32 ptr;
  auto serialized = local_.contents();
  if (!positionCache_.tryFind(serialized, &ptr)) {
    ptr = static_cast<i32>(buffer_->position());
    buffer_->writeStringDataWithoutLengthPrefix(serialized);
    if (!charBuf_.import(&serialized)) {
      return -1;
    }
    positionCache_.insert(serialized, ptr);
    JPP_DCHECK(positionCache_.tryFind(serialized, &ptr));
  }

  return ptr;
}

Status StringKeyValueListFieldImporter::makeStorage(util::CodedBuffer* result) {
  buffer_->writeString("");  // Write 0 as empty sequence
  return StringFieldImporter::makeStorage(result);
}

}  // namespace impl
}  // namespace dic
}  // namespace core
}  // namespace jumanpp
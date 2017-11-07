//
// Created by Arseny Tolmachev on 2017/11/01.
//

#include "dic_feature_impl.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace dic {

using namespace jumanpp::core::impl;

Status ColumnImportContext::initialize(
    i32 index, const spec::FieldDescriptor* descr,
    std::vector<impl::StringStorage>& storages) {
  using namespace core::spec;

  this->index = index;
  this->descriptor = descr;
  this->isTrieIndexed = descr->isTrieKey;

  if (descr->position == 0) {  // autogen
    return Status::Ok();
  }

  auto tp = descr->fieldType;
  auto colpos = descr->position - 1;
  // we want to have original content here
  StringPiece emptyStr = descr->emptyString;

  switch (tp) {
    case FieldType::Int:
      try {
        importer.reset(new impl::IntFieldImporter{colpos});
        break;
      } catch (std::exception& e) {
        return JPPS_INVALID_STATE << "failed to create an IntFieldImporter: "
                                  << e.what();
      }
    case FieldType::String: {
      i32 stringIdx = descr->stringStorage;
      auto stor = &storages[stringIdx];
      importer.reset(new impl::StringFieldImporter{stor, colpos, emptyStr});
      break;
    }
    case FieldType::StringList: {
      i32 stringIdx = descr->stringStorage;
      auto stor = &storages[stringIdx];
      importer.reset(new impl::StringListFieldImporter{stor, colpos, emptyStr});
      break;
    }
    case FieldType::StringKVList: {
      i32 stringIdx = descr->stringStorage;
      auto stor = &storages[stringIdx];
      importer.reset(new impl::StringKeyValueListFieldImporter{
          stor, colpos, emptyStr, descr->listSeparator, descr->kvSeparator});
      break;
    }
    default:
      return Status::NotImplemented()
             << "importing field type=" << tp << " is not implemented";
  }

  return Status::Ok();
}

Status ImportFieldDicFeatureImpl::initialize(
    const DicFeatureContext& ctx, const spec::DicImportDescriptor& desc) {
  if (desc.references.size() != 1) {
    return JPPS_INVALID_PARAMETER << "references size was "
                                  << desc.references.size() << " expected 1";
  }

  importer_ = ctx.importers[desc.references[0]].importer.get();
  target_ = static_cast<u32>(desc.target);

  if (importer_ == nullptr) {
    return JPPS_INVALID_PARAMETER << "failed to find an importer";
  }
  return Status::Ok();
}

Status MatchListDicFeatureImpl::initialize(
    const DicFeatureContext& ctx, const spec::DicImportDescriptor& desc) {
  if (desc.references.size() != 1) {
    return JPPS_INVALID_PARAMETER << "references size was "
                                  << desc.references.size() << " expected 1";
  }

  auto& imp = ctx.importers[desc.references.front()];
  auto& ss = ctx.storage[imp.descriptor->stringStorage];
  storage_ = &ss;
  fieldIdx_ = desc.references.front();
  auto type = imp.descriptor->fieldType;
  switch (type) {
    case spec::FieldType::StringKVList:
      kvSep_ = imp.descriptor->kvSeparator;
    // fallthrough intended
    case spec::FieldType::StringList: {
      auto& sep = imp.descriptor->listSeparator;
      if (sep.size() != 1) {
        return JPPS_INVALID_PARAMETER
               << "list separator must have exactly 1 byte, was [" << sep
               << "], " << sep.size() << " bytes";
      }
      dataSep_ = sep[0];
      break;
    }
    default:
      return JPPS_NOT_IMPLEMENTED
             << "only List and KVList are supported, type was: " << type;
  }

  target_ = static_cast<u32>(desc.target);
  shift_ = static_cast<u32>(desc.shift);

  for (StringPiece v : desc.data) {
    if (!kvSep_.empty()) {
      auto it = std::search(v.begin(), v.end(), kvSep_.begin(), kvSep_.end());
      if (it != v.end()) {
        v = StringPiece{v.begin(), it};
      }
    }
    auto idx = storage_->valueOf(v);
    if (idx == -1) {
      return JPPS_INVALID_PARAMETER << " the value: " << v
                                    << " was not imported";
    }
    data_.insert(idx);
  }
  return Status::Ok();
}

void MatchListDicFeatureImpl::apply(util::MutableArraySlice<i32> featureData,
                                    const util::CsvReader& csv) const {
  auto data = csv.field(fieldIdx_);

  util::CsvReader r{dataSep_};
  r.initFromMemory(data);

  bool contains = false;

  while (r.nextLine()) {
    auto v = r.field(0);
    if (!kvSep_.empty()) {
      auto it = std::search(v.begin(), v.end(), kvSep_.begin(), kvSep_.end());
      if (it != v.end()) {
        v = StringPiece{v.begin(), it};
      }
    }
    auto idx = storage_->valueOf(v);
    if (idx != -1 && data_.contains(idx)) {
      contains = true;
      break;
    }
  }

  if (contains) {
    featureData.at(target_) |= (1 << shift_);
  }
}

Status MatchFieldTupleDicFeatureImpl::initialize(
    const DicFeatureContext& ctx, const spec::DicImportDescriptor& desc) {
  util::copy_insert(desc.references, fields_);
  for (auto f : fields_) {
    auto& o = ctx.importers[f];
    auto& ss = ctx.storage[o.descriptor->stringStorage];
    storages_.push_back(&ss);
  }

  util::CsvReader csv;
  for (auto& s : desc.data) {
    JPP_RETURN_IF_ERROR(csv.initFromMemory(s));
    if (!csv.nextLine()) {
      return JPPS_INVALID_PARAMETER << "could not parse line: " << s;
    }
    if (csv.numFields() != fields_.size()) {
      return JPPS_INVALID_PARAMETER
             << "invalid number of fields: " << csv.numFields() << ", expected "
             << fields_.size();
    }
    DicEntryFeatures def;
    for (int i = 0; i < csv.numFields(); ++i) {
      auto v = csv.field(i);
      auto idx = storages_[i]->valueOf(v);
      if (idx == -1) {
        return JPPS_INVALID_PARAMETER << "in string: " << s
                                      << " the value: " << v
                                      << " was not imported";
      }
      def.features.push_back(idx);
    }
    data_.insert(def);
  }

  target_ = static_cast<u32>(desc.target);
  shift_ = static_cast<u32>(desc.shift);

  if (desc.shift == -1) {
    return JPPS_INVALID_PARAMETER << "shift was not initialized";
  }

  return Status::Ok();
}

void MatchFieldTupleDicFeatureImpl::apply(
    util::MutableArraySlice<i32> featureData,
    const util::CsvReader& csv) const {
  DicEntryFeatures dft;
  for (int i = 0; i < fields_.size(); ++i) {
    auto fldIdx = fields_[i];
    auto storage = storages_[i];
    dft.features.push_back(storage->valueOf(csv.field(fldIdx)));
  }
  if (data_.contains(dft)) {
    featureData.at(target_) |= (1 << shift_);
  }
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp
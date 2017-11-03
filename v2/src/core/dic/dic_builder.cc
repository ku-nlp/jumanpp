//
// Created by Arseny Tolmachev on 2017/02/21.
//

#include "dic_builder.h"
#include "core/dic/dic_build_detail.h"

namespace jumanpp {
namespace core {
namespace dic {

using namespace core::spec;


Status DictionaryBuilder::importCsv(StringPiece name, StringPiece data) {
  if (dic_) {
    return Status::InvalidState() << "dictionary was already built or restored";
  }

  util::CsvReader csv;
  storage_.reset(new DictionaryBuilderStorage);

  JPP_RETURN_IF_ERROR(storage_->initialize(spec_->dictionary));
  JPP_RETURN_IF_ERROR(storage_->initDicFeatures(spec_->features));
  JPP_RETURN_IF_ERROR(storage_->initGroupingFields(*spec_));
  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // first csv pass -- compute stats
  JPP_RETURN_IF_ERROR(storage_->computeStats(name, &csv));

  // build string storage and internal state for the third step
  JPP_RETURN_IF_ERROR(storage_->makeStorage());

  // reinitialize csv for second pass
  csv.reset();

  // second csv pass -- compute entries
  i32 entryCnt = storage_->importActualData(&csv);

  // finally: build trie
  JPP_RETURN_IF_ERROR(storage_->buildTrie());

  // and create answer

  dic_.reset(new BuiltDictionary);
  dic_->entryCount = entryCnt;
  dic_->numFeatures = spec_->features.numDicFeatures;
  dic_->numData = spec_->features.numDicData;
  storage_->fillResult(dic_.get());

  return Status::Ok();
}

Status DictionaryBuilder::importSpec(spec::AnalysisSpec* spec) {
  this->spec_ = spec;
  return Status::Ok();
}

DictionaryBuilder::~DictionaryBuilder() {}

DictionaryBuilder::DictionaryBuilder() {}

template <typename Arch>
void Serialize(Arch& a, BuiltField& o) {
  a& o.uniqueValues;
  a& o.name;
  a& o.emptyValue;
  a& o.colType;
}

template <typename Arch>
void Serialize(Arch& a, BuiltDictionary& o) {
  a& o.entryCount;
  a& o.numFeatures;
  a& o.numData;
  a& o.fieldData;
}

Status DictionaryBuilder::fillModelPart(const RuntimeInfo& info,
                                        model::ModelPart* part) {
  if (!dic_) {
    return Status::InvalidState() << "dictionary is not built yet";
  }

  util::serialization::Saver dicDataSaver{&storage_->builtDicData};
  dicDataSaver.save(*dic_);
  spec::saveSpec(*spec_, &storage_->builtSpecData);
  util::serialization::Saver rtSaver{&storage_->builtRuntimeInfo};
  rtSaver.save(info);

  part->kind = model::ModelPartKind::Dictionary;
  part->data.push_back(storage_->builtDicData.contents());
  part->data.push_back(storage_->builtSpecData.contents());
  part->data.push_back(storage_->builtRuntimeInfo.contents());
  part->data.push_back(dic_->trieContent);
  part->data.push_back(dic_->entryPointers);
  part->data.push_back(dic_->entryData);
  for (auto& sb : storage_->builtStrings) {
    part->data.push_back(sb);
  }
  for (auto& ib : storage_->builtInts) {
    part->data.push_back(ib);
  }

  return Status::Ok();
}

Status DictionaryBuilder::restoreDictionary(const model::ModelInfo& info,
                                            RuntimeInfo* runtime) {
  if (dic_) {
    return Status::InvalidState() << "dictionary was already built or restored";
  }
  dic_.reset(new BuiltDictionary);
  storage_.reset(new DictionaryBuilderStorage);

  i32 dicInfoIdx = -1;
  for (i32 i = 0; i < info.parts.size(); ++i) {
    auto& part = info.parts[i];
    if (part.kind == model::ModelPartKind::Dictionary) {
      if (dicInfoIdx != -1) {
        return Status::InvalidParameter()
               << "saved dictionary had more than one dictionary entries";
      }
      dicInfoIdx = i;
    }
  }

  if (dicInfoIdx == -1) {
    return Status::InvalidParameter()
           << "there was no dictionary information in saved model";
  }

  auto& dicInfo = info.parts[dicInfoIdx];

  if (dicInfo.data.size() < 3) {
    return Status::InvalidParameter() << "dictionary info must have at least "
                                         "three fragments, probably corrupted "
                                         "model file";
  }

  auto builtDicData = dicInfo.data[0];
  auto specData = dicInfo.data[1];
  auto runtimeData = dicInfo.data[2];

  util::serialization::Loader loader{builtDicData};
  if (!loader.load(dic_.get())) {
    return Status::InvalidParameter()
           << "failed to load dictionary metadata from model file";
  }
  loader.reset(specData);
  storage_->restoredSpec.reset(new spec::AnalysisSpec);
  if (!loader.load(storage_->restoredSpec.get())) {
    return Status::InvalidParameter() << "failed to load spec from model file";
  }
  spec_ = storage_->restoredSpec.get();
  loader.reset(runtimeData);
  if (!loader.load(runtime)) {
    return Status::InvalidParameter()
           << "failed to load runtime data from model file";
  }

  return fixupDictionary(dicInfo);
}

Status DictionaryBuilder::fixupDictionary(const model::ModelPart& dicInfo) {
  i32 expectedCount =
      spec_->dictionary.numStringStorage + spec_->dictionary.numIntStorage + 6;
  if (expectedCount != dicInfo.data.size()) {
    return Status::InvalidParameter()
           << "model file did not have all dictionary chunks";
  }
  dic_->trieContent = dicInfo.data[3];
  dic_->entryPointers = dicInfo.data[4];
  dic_->entryData = dicInfo.data[5];
  i32 cnt = 6;
  for (int i = 0; i < spec_->dictionary.numStringStorage; ++i) {
    storage_->builtStrings.push_back(dicInfo.data[cnt]);
    ++cnt;
  }
  for (int i = 0; i < spec_->dictionary.numIntStorage; ++i) {
    storage_->builtInts.push_back(dicInfo.data[cnt]);
    ++cnt;
  }
  if (dic_->fieldData.size() != spec_->dictionary.fields.size()) {
    return Status::InvalidParameter() << "number of columns in spec was not "
                                         "equal to loaded number of columns";
  }
  for (int j = 0; j < dic_->fieldData.size(); ++j) {
    auto& f = dic_->fieldData[j];
    auto& fd = spec_->dictionary.fields[j];
    if (f.name != fd.name) {
      return Status::InvalidParameter()
             << "column name check failed, probably the model is corrupted";
    }
    if (f.colType != fd.fieldType) {
      return Status::InvalidParameter()
             << "column type check for column " << f.name << " failed";
    }
    f.stringStorageIdx = fd.stringStorage;
    f.isSurfaceField = fd.isTrieKey;
    if (fd.stringStorage != -1) {
      f.stringContent = storage_->builtStrings[fd.stringStorage];
    }
    if (fd.intStorage != -1) {
      f.fieldContent = storage_->builtInts[fd.intStorage];
    }
  }
  return Status::Ok();
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

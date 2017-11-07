//
// Created by Arseny Tolmachev on 2017/02/21.
//

#include "dic_builder.h"
#include <chrono>
#include "core/dic/dic_build_detail.h"
#include "core/spec/spec_ser.h"

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
  JPP_RETURN_IF_ERROR(storage_->initGroupingFields(*spec_));
  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // first csv pass -- compute stats
  JPP_RETURN_IF_ERROR(storage_->computeStats(name, &csv));

  // build string storage and internal state for the third step
  JPP_RETURN_IF_ERROR(storage_->makeStorage());

  // reinitialize csv for second pass
  csv.reset();

  JPP_RETURN_IF_ERROR(storage_->initDicFeatures(spec_->features));

  // second csv pass -- compute entries
  i32 entryCnt = storage_->importActualData(&csv);

  // finally: build trie
  JPP_RETURN_IF_ERROR(storage_->buildTrie());

  // and create answer

  dic_.reset(new BuiltDictionary);
  dic_->spec = *spec_;
  dic_->entryCount = entryCnt;
  auto time = std::chrono::system_clock::now();
  auto seconds =
      std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());
  dic_->timestamp = seconds.count();
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
  a& o.dicIndex;
  a& o.specIndex;
  a& o.uniqueValues;
}

template <typename Arch>
void Serialize(Arch& a, BuiltDictionary& o) {
  a& o.entryCount;
  a& o.fieldData;
  a& o.comment;
  a& o.timestamp;
  a& o.spec;
}

Status DictionaryBuilder::fillModelPart(model::ModelPart* part) {
  if (!dic_) {
    return Status::InvalidState() << "dictionary is not built yet";
  }

  util::serialization::Saver dicDataSaver{&storage_->builtDicData};
  dicDataSaver.save(*dic_);

  part->kind = model::ModelPartKind::Dictionary;
  part->data.push_back(storage_->builtDicData.contents());
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

inline Status fixupDictionary(const model::ModelPart& dicInfo,
                              BuiltDictionary* dic) {
  auto spec_ = &dic->spec;
  i32 expectedCount =
      spec_->dictionary.numStringStorage + spec_->dictionary.numIntStorage + 4;
  if (expectedCount != dicInfo.data.size()) {
    return Status::InvalidParameter()
           << "model file did not have all dictionary chunks";
  }
  dic->trieContent = dicInfo.data[1];
  dic->entryPointers = dicInfo.data[2];
  dic->entryData = dicInfo.data[3];
  i32 cnt = 4;
  for (int i = 0; i < spec_->dictionary.numStringStorage; ++i) {
    dic->stringStorages.push_back(dicInfo.data[cnt]);
    ++cnt;
  }
  for (int i = 0; i < spec_->dictionary.numIntStorage; ++i) {
    dic->intStorages.push_back(dicInfo.data[cnt]);
    ++cnt;
  }
  if (dic->fieldData.size() != spec_->dictionary.fields.size()) {
    return Status::InvalidParameter() << "number of columns in spec was not "
                                         "equal to loaded number of columns";
  }
  for (int j = 0; j < dic->fieldData.size(); ++j) {
    auto& f = dic->fieldData[j];
    auto& fd = spec_->dictionary.fields[f.specIndex];
    if (f.dicIndex != fd.dicIndex) {
      return JPPS_INVALID_PARAMETER << "something went wrong and built field "
                                       "dicIndex !=  spec field dicIndex: "
                                    << f.dicIndex << " vs " << fd.dicIndex;
    }
    f.stringStorageIdx = fd.stringStorage;
    if (fd.stringStorage != spec::InvalidInt) {
      f.stringContent = dic->stringStorages[fd.stringStorage];
    }
    if (fd.intStorage != spec::InvalidInt) {
      f.fieldContent = dic->intStorages[fd.intStorage];
    }
  }
  return Status::Ok();
}

Status BuiltDictionary::restoreDictionary(const model::ModelInfo& info) {
  i32 dicInfoIdx = -1;
  for (i32 i = 0; i < info.parts.size(); ++i) {
    auto& part = info.parts[i];
    if (part.kind == model::ModelPartKind::Dictionary) {
      if (dicInfoIdx != -1) {
        return JPPS_INVALID_PARAMETER
               << "saved dictionary had more than one dictionary entries";
      }
      dicInfoIdx = i;
    }
  }

  if (dicInfoIdx == -1) {
    return JPPS_INVALID_PARAMETER
           << "there was no dictionary information in saved model";
  }

  auto& dicInfo = info.parts[dicInfoIdx];

  if (dicInfo.data.size() < 2) {
    return Status::InvalidParameter() << "dictionary info must have at least "
                                         "two fragments, probably corrupted "
                                         "model file";
  }

  auto builtDicData = dicInfo.data[0];

  util::serialization::Loader loader{builtDicData};
  // corrupt magic fields
  spec.specMagic_ = (u32)spec::InvalidInt;
  spec.specMagic2_ = spec.specMagic_;
  spec.specVersion_ = spec.specMagic_;

  if (!loader.load(this)) {
    return JPPS_INVALID_PARAMETER
           << "failed to load dictionary metadata from model file";
  }

  JPP_RETURN_IF_ERROR(spec.validate());

  return fixupDictionary(dicInfo, this);
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

//
// Created by Arseny Tolmachev on 2017/11/01.
//

#include "dic_build_detail.h"
#include "dic_builder.h"

namespace jumanpp {
namespace core {
namespace dic {

using namespace core::spec;

void DictionaryBuilderStorage::fillResult(BuiltDictionary* dic_) {
  for (auto& b : stringBuffers) {
    builtStrings.push_back(b.contents());
  }
  for (auto& b : intBuffers) {
    builtInts.push_back(b.contents());
  }

  dic_->trieContent = entries.trieBuilder.daBuilder.result();
  dic_->entryPointers = entries.trieBuilder.entryPtrBuffer.contents();
  dic_->entryData = entries.entryDataBuffer.contents();
  auto& flds = dic_->fieldData;
  for (auto& i : importers) {
    BuiltField fld;
    fld.name = i.descriptor->name;
    fld.emptyValue = i.descriptor->emptyString;
    fld.stringStorageIdx = i.descriptor->stringStorage;
    if (i.descriptor->stringStorage != -1) {
      fld.stringContent = builtStrings[i.descriptor->stringStorage];
    }
    if (i.descriptor->intStorage != -1) {
      fld.fieldContent = builtInts[i.descriptor->intStorage];
    }
    fld.colType = i.descriptor->fieldType;
    fld.isSurfaceField = i.descriptor->isTrieKey;
    fld.uniqueValues = i.importer->uniqueValues();
    flds.push_back(fld);
  }
}

void DictionaryBuilderStorage::importSpecData(const s::AnalysisSpec& spec) {
  for (auto& f : spec.features.primitive) {
    for (auto fldIdx : f.references) {
      if (f.matchData.empty()) {
        continue;
      }
      auto ss = importers[fldIdx].descriptor->stringStorage;
      if (ss != -1) {
        auto& stor = storage[ss];
        for (auto& s : f.matchData) {
          stor.increaseFieldValueCount(s);
        }
      }
    }
  }

  for (auto& f : spec.features.computation) {
    auto& data = f.matchData;
    auto& refs = f.matchReference;
    auto refSize = refs.size();
    for (int i = 0; i < data.size(); ++i) {
      auto refIdx = i % refSize;
      auto& ref = refs[refIdx];
      auto descr = importers[ref.dicFieldIdx].descriptor;
      auto ss = descr->stringStorage;
      if (ss != -1) {
        auto obj = data[i];
        if (descr->emptyString != obj) {
          storage[ss].increaseFieldValueCount(obj);
        }
      }
    }
  }
}

Status DictionaryBuilderStorage::computeStats(StringPiece name,
                                              util::CsvReader* csv) {
  while (csv->nextLine()) {
    auto ncols = csv->numFields();
    if (maxUsedCol >= ncols) {
      return Status::InvalidParameter()
             << "when processing file: " << name << ", on line "
             << csv->lineNumber() << " there were " << ncols
             << " columns, however field " << maxFieldName
             << " is defined as column #" << maxUsedCol + 1;
    }

    for (auto& imp : importers) {
      if (!imp.importFieldValue(*csv)) {
        return Status::InvalidState()
               << "when processing dictionary file " << name
               << " import failed when importing column number "
               << imp.descriptor->position << " named " << imp.descriptor->name
               << " line #" << csv->lineNumber();
      }
      if (imp.isTrieIndexed) {
        entries.addSurface(csv->field(imp.descriptor->position - 1));
      }
    }
  }
  return Status::Ok();
}

Status DictionaryBuilderStorage::makeStorage() {
  for (int i = 0; i < storage.size(); ++i) {
    storage[i].makeStorage(&stringBuffers[i]);
  }

  for (auto& imp : importers) {
    auto descriptor = imp.descriptor;

    auto intNum = descriptor->intStorage;
    if (intNum != -1) {
      auto buffer = &intBuffers[intNum];
      imp.importer->injectFieldBuffer(buffer);
    }
  }
  return Status::Ok();
}

i32 DictionaryBuilderStorage::importActualData(util::CsvReader* csv) {
  i32 entryCnt = 0;
  while (csv->nextLine()) {
    entries.importOneLine(importers, *csv);
    entryCnt += 1;
  }
  return entryCnt;
}

Status DictionaryBuilderStorage::buildTrie() {
  if (indexColumn == -1) {
    return Status::InvalidParameter() << "index column was not specified";
  }
  auto storageIdx = importers[indexColumn].descriptor->stringStorage;
  auto& stringBuf = storage[storageIdx];
  JPP_RETURN_IF_ERROR(entries.trieBuilder.buildTrie(stringBuf));
  return Status::Ok();
}

Status DictionaryBuilderStorage::initialize(const s::DictionarySpec& dicSpec) {
  indexColumn = dicSpec.indexColumn;

  importers.resize(dicSpec.fields.size());
  storage.resize(dicSpec.numStringStorage);
  stringBuffers.resize(dicSpec.numStringStorage);
  intBuffers.resize(dicSpec.numIntStorage);

  for (int i = 0; i < dicSpec.fields.size(); ++i) {
    auto& column = dicSpec.fields[i];
    JPP_RETURN_IF_ERROR(importers.at(i).initialize(i, &column, storage));
    auto colIdx = column.position - 1;
    if (maxUsedCol < colIdx) {
      maxUsedCol = colIdx;
      maxFieldName = column.name;
    }
  }

  return Status::Ok();
}

Status DictionaryBuilderStorage::initDicFeatures(const FeaturesSpec& dicSpec) {
  DicFeatureContext ctx{storage, importers, stringBuffers, intBuffers};
  JPP_RETURN_IF_ERROR(entries.createFeatures(dicSpec, ctx));
  entries.surfaceIdx_ = static_cast<u32>(indexColumn);
  auto desc = importers[indexColumn].descriptor;
  entries.realSurfaceStorage_ = &storage[desc->stringStorage];
  entries.featureBuf_.resize(dicSpec.numDicFeatures);
  return Status::Ok();
}

Status DictionaryBuilderStorage::initGroupingFields(
    const s::AnalysisSpec& spec) {
  return Status::Ok();
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp
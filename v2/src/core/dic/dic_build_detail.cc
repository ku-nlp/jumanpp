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
    fld.dicIndex = i.descriptor->dicIndex;
    fld.specIndex = i.descriptor->specIndex;
    fld.stringStorageIdx = i.descriptor->stringStorage;
    if (i.descriptor->stringStorage != InvalidInt) {
      fld.stringContent = builtStrings[i.descriptor->stringStorage];
    }
    if (i.descriptor->intStorage != InvalidInt) {
      fld.fieldContent = builtInts[i.descriptor->intStorage];
    }
    if (i.importer) {
      fld.uniqueValues = i.importer->uniqueValues();
    }
    flds.push_back(fld);
  }

  std::sort(flds.begin(), flds.end(),
            [](const BuiltField& f1, const BuiltField& f2) {
              i32 modIdx1 = f1.dicIndex ^ 0x8000'0000;
              i32 modIdx2 = f2.dicIndex ^ 0x8000'0000;
              return modIdx1 < modIdx2;
            });

  for (auto& ss : stringBuffers) {
    dic_->stringStorages.push_back(ss.contents());
  }

  for (auto& ib : intBuffers) {
    dic_->intStorages.push_back(ib.contents());
  }

  for (auto& unk : dic_->spec.unkCreators) {
    auto iter = entries.ignoredRows.find(unk.patternRow);
    if (iter != entries.ignoredRows.end()) {
      unk.patternPtr = iter->second;
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
    if (intNum != spec::InvalidInt) {
      auto buffer = &intBuffers[intNum];
      imp.importer->injectFieldBuffer(buffer);
    }
  }
  return Status::Ok();
}

i32 DictionaryBuilderStorage::importActualData(util::CsvReader* csv) {
  i32 entryCnt = 0;
  while (csv->nextLine()) {
    entryCnt += entries.importOneLine(importers, *csv);
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
  for (auto idx : spec.dictionary.aliasingSet) {
    auto& fld = spec.dictionary.fields[idx];
    auto dicIdx = fld.dicIndex;
    if (dicIdx < 0) {
      return JPPS_INVALID_PARAMETER << "field " << fld.name
                                    << " could not be used for deduplication";
    }
    JPP_DCHECK_IN(dicIdx, 0, spec.features.numDicFeatures);
    entries.dedupIdxes_.push_back(dicIdx);
  }

  for (auto& unk : spec.unkCreators) {
    entries.ignoredRows[unk.patternRow] = InvalidInt;
  }

  return Status::Ok();
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp
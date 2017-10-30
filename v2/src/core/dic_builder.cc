//
// Created by Arseny Tolmachev on 2017/02/21.
//

#include "core/dic_builder.h"
#include <memory>
#include <util/status.hpp>
#include <vector>
#include "core/darts_trie.h"
#include "core/impl/field_import.h"
#include "core/impl/field_reader.h"
#include "core/impl/runtime_ser.h"
#include "core/spec/spec_types.h"
#include "spec/spec_serialization.h"
#include "util/coded_io.h"
#include "util/csv_reader.h"
#include "util/flatmap.h"
#include "util/flatset.h"
#include "util/inlined_vector.h"
#include "util/serialization.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace dic {

using namespace core::spec;

struct ColumnImportContext {
  i32 index;
  const FieldDescriptor* descriptor;
  bool isTrieIndexed = false;
  std::unique_ptr<impl::FieldImporter> importer;

  Status initialize(i32 index, const FieldDescriptor* descr,
                    std::vector<impl::StringStorage>& storages) {
    this->index = index;
    this->descriptor = descr;
    this->isTrieIndexed = descr->isTrieKey;

    auto tp = descr->columnType;
    auto colpos = descr->position - 1;
    // we want to have original content here
    StringPiece emptyStr = descr->emptyString;

    switch (tp) {
      case ColumnType::Int:
        importer.reset(new impl::IntFieldImporter{colpos});
      case ColumnType::String: {
        i32 stringIdx = descr->stringStorage;
        auto stor = &storages[stringIdx];
        importer.reset(new impl::StringFieldImporter{stor, colpos, emptyStr});
        break;
      }
      case ColumnType::StringList: {
        i32 stringIdx = descr->stringStorage;
        auto stor = &storages[stringIdx];
        importer.reset(
            new impl::StringListFieldImporter{stor, colpos, emptyStr});
        break;
      }
      case ColumnType::StringKVList: {
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

  bool importFieldValue(const util::CsvReader& reader) {
    return importer->importFieldValue(reader);
  }
};

struct DicTrieBuilder {
  util::CodedBuffer entryPtrBuffer;
  util::FlatMap<i32, util::InlinedVector<i32, 4>> entriesWithField;
  DoubleArrayBuilder daBuilder;

  void addEntry(i32 fieldValue, i32 entryPtr) {
    entriesWithField[fieldValue].push_back(entryPtr);
  }

  Status buildTrie(const impl::StringStorage& strings) {
    for (auto& v : strings) {
      StringPiece key = v.first;
      i32 keyPtr = v.second;
      auto it = entriesWithField.find(keyPtr);
      if (it != entriesWithField.end()) {
        auto& entries = it->second;
        auto entriesPtr = static_cast<i32>(entryPtrBuffer.position());
        impl::writePtrsAsDeltas(entries, entryPtrBuffer);
        daBuilder.add(key, entriesPtr);
      }
    }
    return daBuilder.build();
  }
};

struct EntryTableBuilder {
  /**
   * This set contains row numbers which serve patterns for UNK builders
   */
  util::FlatSet<i32> ignoredRows;
  util::CodedBuffer entryDataBuffer;
  DicTrieBuilder trieBuilder;

  i32 importOneLine(std::vector<ColumnImportContext>& columns,
                    const util::CsvReader& csv) {
    auto ptr = entryDataBuffer.position();
    auto iptr = static_cast<i32>(ptr);
    for (auto& c : columns) {
      auto field = c.importer->fieldPointer(csv);
      auto uns = static_cast<u32>(field);
      entryDataBuffer.writeVarint(uns);
      if (field != 0 && c.isTrieIndexed) {
        if (ignoredRows.count(csv.lineNumber()) == 0) {
          trieBuilder.addEntry(field, iptr);
        }
      }
    }
    return iptr;
  }
};

struct DictionaryBuilderStorage {
  std::vector<impl::StringStorage> storage;
  std::vector<ColumnImportContext> importers;
  std::vector<util::CodedBuffer> stringBuffers;
  std::vector<util::CodedBuffer> intBuffers;
  EntryTableBuilder entries;
  util::CodedBuffer builtDicData;
  util::CodedBuffer builtSpecData;
  util::CodedBuffer builtRuntimeInfo;
  std::unique_ptr<spec::AnalysisSpec> restoredSpec;
  std::vector<StringPiece> builtStrings;
  std::vector<StringPiece> builtInts;

  i32 maxUsedCol = -1;
  StringPiece maxFieldName;
  i32 indexColumn = -1;

  Status initialize(const DictionarySpec& dicSpec) {
    indexColumn = dicSpec.indexColumn;

    importers.resize(dicSpec.columns.size());
    storage.resize(dicSpec.numStringStorage);
    stringBuffers.resize(dicSpec.numStringStorage);
    intBuffers.resize(dicSpec.numIntStorage);

    for (int i = 0; i < dicSpec.columns.size(); ++i) {
      const FieldDescriptor& column = dicSpec.columns[i];
      JPP_RETURN_IF_ERROR(importers.at(i).initialize(i, &column, storage));
      auto colIdx = column.position - 1;
      if (maxUsedCol < colIdx) {
        maxUsedCol = colIdx;
        maxFieldName = column.name;
      }
    }

    return Status::Ok();
  }

  Status computeStats(StringPiece name, util::CsvReader* csv) {
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
                 << imp.descriptor->position << " named "
                 << imp.descriptor->name << " line #" << csv->lineNumber();
        }
      }
    }
    return Status::Ok();
  }

  Status makeStorage() {
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

  i32 importActualData(util::CsvReader* csv) {
    i32 entryCnt = 0;
    while (csv->nextLine()) {
      entries.importOneLine(importers, *csv);
      entryCnt += 1;
    }
    return entryCnt;
  }

  Status buildTrie() {
    if (indexColumn == -1) {
      return Status::InvalidParameter() << "index column was not specified";
    }
    auto storageIdx = importers[indexColumn].descriptor->stringStorage;
    auto& stringBuf = storage[storageIdx];
    JPP_RETURN_IF_ERROR(entries.trieBuilder.buildTrie(stringBuf));
    return Status::Ok();
  }

  void fillResult(BuiltDictionary* dic_) {
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
      fld.colType = i.descriptor->columnType;
      fld.isSurfaceField = i.descriptor->isTrieKey;
      fld.uniqueValues = i.importer->uniqueValues();
      flds.push_back(fld);
    }
  }

  void importSpecData(const AnalysisSpec& spec) {
    for (auto& x : spec.unkCreators) {
      entries.ignoredRows.insert(x.patternRow);
      for (auto& ex : x.outputExpressions) {
        auto ss = importers[ex.fieldIndex].descriptor->stringStorage;
        if (ex.stringConstant.size() > 0 && ss != -1) {
          storage[ss].increaseFieldValueCount(ex.stringConstant);
        }
      }
    }

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
};

Status DictionaryBuilder::importCsv(StringPiece name, StringPiece data) {
  if (dic_) {
    return Status::InvalidState() << "dictionary was already built or restored";
  }

  util::CsvReader csv;
  storage_.reset(new DictionaryBuilderStorage);

  JPP_RETURN_IF_ERROR(storage_->initialize(spec_->dictionary));
  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // first csv pass -- compute stats
  JPP_RETURN_IF_ERROR(storage_->computeStats(name, &csv));
  storage_->importSpecData(*spec_);

  // build string storage and internal state for the third step
  JPP_RETURN_IF_ERROR(storage_->makeStorage());

  // reinitialize csv for second pass
  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // second csv pass -- compute entries
  i32 entryCnt = storage_->importActualData(&csv);

  // finally: build trie
  JPP_RETURN_IF_ERROR(storage_->buildTrie());

  // and create answer

  dic_.reset(new BuiltDictionary);
  dic_->entryCount = entryCnt;
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
  if (dic_->fieldData.size() != spec_->dictionary.columns.size()) {
    return Status::InvalidParameter() << "number of columns in spec was not "
                                         "equal to loaded number of columns";
  }
  for (int j = 0; j < dic_->fieldData.size(); ++j) {
    auto& f = dic_->fieldData[j];
    auto& fd = spec_->dictionary.columns[j];
    if (f.name != fd.name) {
      return Status::InvalidParameter()
             << "column name check failed, probably the model is corrupted";
    }
    if (f.colType != fd.columnType) {
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

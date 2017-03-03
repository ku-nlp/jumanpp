//
// Created by Arseny Tolmachev on 2017/02/21.
//

#include "core/dic_builder.h"
#include <util/string_piece.h>
#include <memory>
#include <util/status.hpp>
#include <vector>
#include "core/darts_trie.h"
#include "core/impl/field_import.h"
#include "core/impl/field_reader.h"
#include "core/spec/spec_types.h"
#include "util/coded_io.h"
#include "util/csv_reader.h"
#include "util/flatmap.h"
#include "util/inlined_vector.h"

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
    auto emptyStr = descr->emptyString;

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
      default:
        return Status::NotImplemented() << "importing field type=" << tp
                                        << " is not implemented";
    }

    return Status::Ok();
  }

  bool importFieldValue(const util::CsvReader& reader) {
    return importer->importFieldValue(reader);
  }
};

struct DicTrieBuilder {
  util::CodedBuffer buffer;
  util::FlatMap<i32, util::InlinedVector<i32, 4>> entriesWithField;
  DoubleArrayBuilder daBuilder;

  void addEntry(i32 fieldValue, i32 entryPtr) {
    entriesWithField[fieldValue].push_back(entryPtr);
  }

  Status buildTrie(const impl::StringStorage& strings) {
    for (auto& v : strings) {
      StringPiece key = v.first;
      i32 keyPtr = v.second;
      auto& entries = entriesWithField[keyPtr];
      auto entriesPtr = static_cast<i32>(buffer.position());
      impl::writePtrsAsDeltas(entries, buffer);
      daBuilder.add(key, entriesPtr);
    }
    return daBuilder.build();
  }
};

struct EntryTableBuilder {
  util::CodedBuffer buffer;
  DicTrieBuilder trieBuilder;

  i32 importOneLine(std::vector<ColumnImportContext>& columns,
                    const util::CsvReader& csv) {
    auto ptr = buffer.position();
    auto iptr = static_cast<i32>(ptr);
    for (auto& c : columns) {
      auto field = c.importer->fieldPointer(csv);
      auto uns = static_cast<u32>(field);
      buffer.writeVarint(uns);
      if (c.isTrieIndexed) {
        trieBuilder.addEntry(field, iptr);
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
               << " is defined as " << maxUsedCol << "th column";
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
    dic_->trieContent = entries.trieBuilder.daBuilder.result();
    dic_->entryPointers = entries.trieBuilder.buffer.contents();
    dic_->entryData = entries.buffer.contents();
    auto& flds = dic_->fieldData;
    for (auto& i : importers) {
      BuiltField fld;
      fld.name = i.descriptor->name;
      if (i.descriptor->stringStorage != -1) {
        fld.stringContent = stringBuffers[i.descriptor->stringStorage].contents();
      }
      if (i.descriptor->intStorage != -1) {
        fld.fieldContent = intBuffers[i.descriptor->intStorage].contents();
      }
      fld.colType = i.descriptor->columnType;
      fld.uniqueValues = i.importer->uniqueValues();
      flds.push_back(fld);
    }
  }
};

Status DictionaryBuilder::importCsv(StringPiece name, StringPiece data) {
  util::CsvReader csv;
  storage_.reset(new DictionaryBuilderStorage);

  JPP_RETURN_IF_ERROR(storage_->initialize(spec_->dictionary));
  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // first csv pass -- compute stats
  JPP_RETURN_IF_ERROR(storage_->computeStats(name, &csv));

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

}  // dic
}  // core
}  // jumanpp

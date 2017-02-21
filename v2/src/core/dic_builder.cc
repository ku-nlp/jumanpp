//
// Created by Arseny Tolmachev on 2017/02/21.
//

#include "dic_builder.h"
#include <util/coded_io.h>
#include <util/csv_reader.h>
#include <util/flatmap.h>
#include <util/inlined_vector.h>
#include <util/string_piece.h>
#include <memory>
#include <util/status.hpp>
#include <vector>
#include "core/darts_trie.h"
#include "core/impl/field_import.h"
#include "core/impl/field_reader.h"

namespace jumanpp {
namespace core {
namespace dic {

struct ColumnImportContext {
  i32 index;
  const ColumnDescriptor* descriptor;
  util::CodedBuffer stringData;
  util::CodedBuffer fieldData;
  bool useFieldData = false;
  bool isTrieIndexed = false;
  std::unique_ptr<impl::FieldImporter> importer;

  Status initialize(i32 index, const ColumnDescriptor* descr) {
    this->index = index;
    this->descriptor = descr;
    this->isTrieIndexed = descr->isTrieKey;

    auto tp = descr->columnType;

    switch (tp) {
      case ColumnType::String:
        importer.reset(new impl::StringFieldImporter{descr->position - 1});
        break;
      case ColumnType::StringList:
        importer.reset(new impl::StringListFieldImporter{descr->position - 1});
      default:
        return Status::NotImplemented()
               << "importing field type=" << static_cast<int>(tp)
               << " is not implemented";
    }

    if (importer->requiresFieldBuffer()) {
      importer->injectFieldBuffer(&fieldData);
      useFieldData = true;
    }

    return Status::Ok();
  }

  i32 numBuffers() const { return useFieldData ? 2 : 1; }

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

  Status buildTrie(const impl::StringStorageReader& strings) {
    for (auto& v : entriesWithField) {
      i32 keyPtr = v.first;
      auto& values = v.second;
      JPP_DCHECK_GE(values.size(), 0);
      auto entriesPtr = static_cast<i32>(buffer.position());
      impl::writePtrsAsDeltas(values, buffer);
      StringPiece sp;
      if (!strings.readAt(keyPtr, &sp)) {
        return Status::InvalidState()
               << " trie index field was corrupted on construction, could not "
                  "find key for ptr="
               << keyPtr;
      }
      daBuilder.add(sp, entriesPtr);
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
  std::vector<ColumnImportContext> importers;
  EntryTableBuilder entries;
};

Status DictionaryBuilder::importCsv(StringPiece name, StringPiece data) {
  util::CsvReader csv;

  i32 maxUsedCol = -1;
  StringPiece maxFieldName;
  storage_.reset(new DictionaryBuilderStorage);
  auto& importers = storage_->importers;
  importers.resize(spec_->columns.size());

  for (int i = 0; i < spec_->columns.size(); ++i) {
    const ColumnDescriptor& column = spec_->columns[i];
    JPP_RETURN_IF_ERROR(importers[i].initialize(i, &column));
    auto colIdx = column.position - 1;
    if (maxUsedCol < colIdx) {
      maxUsedCol = colIdx;
      maxFieldName = column.name;
    }
  }

  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // first csv pass -- compute stats
  while (csv.nextLine()) {
    auto ncols = csv.numFields();
    if (maxUsedCol >= ncols) {
      return Status::InvalidParameter()
             << "when processing file: " << name << ", on line "
             << csv.lineNumber() << " there were " << ncols
             << " columns, however field " << maxFieldName << " is defined as "
             << maxUsedCol << "th column";
    }

    for (auto& imp : importers) {
      if (!imp.importFieldValue(csv)) {
        return Status::InvalidState()
               << "when processing dictionary file " << name
               << " import failed when importing column number "
               << imp.descriptor->position << " named " << imp.descriptor->name
               << " line #" << csv.lineNumber();
      }
    }
  }

  // build string storage and internal state for the third step
  for (auto& imp : importers) {
    JPP_RETURN_IF_ERROR(imp.importer->makeStorage(&imp.stringData));
  }

  JPP_RETURN_IF_ERROR(csv.initFromMemory(data));

  // second csv pass -- compute entries

  auto& entries = storage_->entries;

  i32 entryCnt = 0;
  while (csv.nextLine()) {
    entries.importOneLine(importers, csv);
    entryCnt += 1;
  }

  // finally: build trie

  StringPiece sp;
  for (auto& i : importers) {
    if (i.isTrieIndexed) {
      if (sp.size() == 0) {
        sp = i.stringData.contents();
      } else {
        return Status::InvalidState()
               << "there was more than one field to index in trie";
      }
    }
  }

  if (sp.size() == 0) {
    return Status::InvalidState()
           << "could not build trie because string storage was empty";
  }

  JPP_RETURN_IF_ERROR(entries.trieBuilder.buildTrie(sp));

  // and create answer

  dic_.reset(new BuiltDictionary);
  dic_->entryCount = entryCnt;
  dic_->dicSpec = specContent_;
  dic_->trieContent = entries.trieBuilder.daBuilder.result();
  dic_->entryPointers = entries.trieBuilder.buffer.contents();
  dic_->entryData = entries.buffer.contents();
  auto& flds = dic_->fieldData;
  for (auto& i : importers) {
    BuiltField fld{i.descriptor->name, i.stringData.contents(),
                   i.fieldData.contents(), i.useFieldData,
                   i.importer->uniqueValues()};
    flds.push_back(fld);
  }

  return Status::Ok();
}

Status DictionaryBuilder::importSpec(StringPiece name, StringPiece data) {
  spec_.reset(new DictionarySpec);
  specContent_.append(data.begin(), data.end());
  return parseDescriptor(data, spec_.get());
}

DictionaryBuilder::~DictionaryBuilder() {}

DictionaryBuilder::DictionaryBuilder() {}

}  // dic
}  // core
}  // jumanpp

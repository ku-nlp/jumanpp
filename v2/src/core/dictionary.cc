//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"
#include "core/impl/field_import.h"

#include <util/coded_io.h>
#include <util/csv_reader.h>
#include <util/flatmap.h>
#include <util/inlined_vector.h>
#include <util/string_piece.h>
#include <memory>
#include <vector>
#include "core/darts_trie.h"
#include "core/impl/field_reader.h"

namespace jumanpp {
namespace core {
namespace dic {

Status EntryDescriptor::build(const DictionarySpec& spec) {
  spec_ = &spec;

  return Status::Ok();
}

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
      buffer.writeVarint(ptr);
      if (c.isTrieIndexed) {
        trieBuilder.addEntry(field, iptr);
      }
    }
    return iptr;
  }
};

Status DictionaryBuilder::importFromMemory(StringPiece data, StringPiece name) {
  util::CsvReader csv;

  i32 maxUsedCol = -1;
  StringPiece maxFieldName;
  std::vector<ColumnImportContext> importers;
  importers.resize(spec_->columns.size());

  for (int i = 0; i < spec_->columns.size(); ++i) {
    const ColumnDescriptor& column = spec_->columns[i];
    JPP_RETURN_IF_ERROR(importers[i].initialize(i, &column));
    auto colIdx = column.position;
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
             << "when processing file: " << name << " there were " << ncols
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

  EntryTableBuilder entries;

  while (csv.nextLine()) {
    entries.importOneLine(importers, csv);
  }

  return Status::Ok();
}

}  // dic
}  // core
}  // jumanpp

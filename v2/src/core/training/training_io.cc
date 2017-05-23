//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "training_io.h"

namespace jumanpp {
namespace core {
namespace training {

Status readStr2IdMap(const dic::DictionaryField &fld,
                     util::FlatMap<StringPiece, i32> *map) {
  auto &s2i = *map;

  if (fld.columnType != spec::ColumnType::String) {
    return Status::InvalidParameter()
           << "training data reader allows only string-typed fields, field "
           << fld.name << " was not";
  }

  if (fld.emptyValue.size() != 0) {
    s2i[fld.emptyValue] = 0;
  }

  core::dic::impl::StringStorageTraversal everything{fld.strings.data()};
  StringPiece data;

  while (everything.next(&data)) {
    s2i[data] = everything.position();
  }

  return Status::Ok();
}

Status TrainingDataReader::initialize(const spec::TrainingSpec &spec,
                                      const CoreHolder &core) {
  storages_.resize(spec.fields.size());
  for (i32 i = 0; i < spec.fields.size(); ++i) {
    auto &tf = spec.fields[i];
    auto &dicFld = core.dic().fields().at(tf.fieldIdx);
    auto &str2int = storages_[dicFld.stringStorageIdx];

    if (str2int.size() == 0) {
      // string storage was not read yet
      JPP_RETURN_IF_ERROR(readStr2IdMap(dicFld, &str2int));
    }

    fields_.push_back({&str2int, tf.fieldIdx, i});
  }
  surfaceFieldIdx_ = spec.fields[spec.surfaceIdx].number;
  return Status::Ok();
}

Status TrainingDataReader::readFullExample(analysis::ExtraNodesContext *xtra,
                                           FullyAnnotatedExample *result) {
  result->data_.clear();
  result->lengths_.clear();

  switch (mode_) {
    case DataReaderMode::SimpleCsv:
      return readFullExampleCsv(xtra, result);
    case DataReaderMode::DoubleCsv:
      return readFullExampleDblCsv(xtra, result);
    default:
      return Status::NotImplemented()
             << "example type " << (int)mode_ << " is not implemented";
  }
}

Status TrainingDataReader::initCsv(StringPiece data) {
  if (fields_.size() == 0) {
    return Status::InvalidState()
           << "field data is not initialized, you must do that first";
  }

  mode_ = DataReaderMode::SimpleCsv;
  csv_ = util::CsvReader();
  finished_ = false;
  return csv_.initFromMemory(data);
}

Status TrainingDataReader::initDoubleCsv(StringPiece data, char tokenSep,
                                         char fieldSep) {
  if (fields_.size() == 0) {
    return Status::InvalidState()
           << "field data is not initialized, you must do that first";
  }

  mode_ = DataReaderMode::DoubleCsv;
  doubleFldSep_ = fieldSep;
  csv_ = util::CsvReader{tokenSep};
  csv2_ = util::CsvReader{fieldSep};
  finished_ = false;
  return csv_.initFromMemory(data);
}

Status TrainingDataReader::readFullExampleCsv(analysis::ExtraNodesContext *xtra,
                                              FullyAnnotatedExample *result) {
  while (csv_.nextLine()) {
    if (csv_.numFields() == 1 && csv_.field(0).size() == 0) {
      return Status::Ok();
    }

    readSingleExampleFragment(csv_, xtra, result);
  }
  finished_ = true;
  return Status::Ok();
}

Status TrainingDataReader::readFullExampleDblCsv(
    analysis::ExtraNodesContext *xtra, FullyAnnotatedExample *result) {
  finished_ = !csv_.nextLine();
  if (finished_) {
    return Status::Ok();
  }

  auto numwords = csv_.numFields();
  result->data_.reserve(numwords * fields_.size());
  result->lengths_.reserve(numwords);
  for (int i = 0; i < numwords; ++i) {
    auto content = csv_.field(i);

    if (content == "#") {
      for (int j = i + 1; j < numwords; ++j) {
        auto commentPart = csv_.field(j);
        result->comment_.append(commentPart.char_begin(),
                                commentPart.char_end());
        if (j != numwords - 1) {
          result->comment_.push_back(csv_.separator());
        }
      }
      break;
    }

    JPP_RETURN_IF_ERROR(csv2_.initFromMemory(content));
    if (!csv2_.nextLine()) {
      return Status::InvalidParameter()
             << "failed to read word #" << i << " from the line #"
             << csv_.lineNumber();
    }
    JPP_RETURN_IF_ERROR(readSingleExampleFragment(csv2_, xtra, result));
  }

  return Status::Ok();
}

Status TrainingDataReader::readSingleExampleFragment(
    const util::CsvReader &csv, analysis::ExtraNodesContext *xtra,
    FullyAnnotatedExample *result) {
  codepts_.clear();
  auto surfFld = csv.field(surfaceFieldIdx_);

  JPP_RETURN_IF_ERROR(chars::preprocessRawData(surfFld, &codepts_));
  result->lengths_.push_back(codepts_.size());

  if (csv.numFields() < fields_.size()) {
    return Status::InvalidParameter()
           << "a word from the line #" << csv_.lineNumber() << " had "
           << csv.numFields() << " fields, expected " << fields_.size();
  }
  result->surface_.append(surfFld.char_begin(), surfFld.char_end());
  for (int i = 0; i < fields_.size(); ++i) {
    auto &fldInfo = fields_[i];
    auto &map = *fldInfo.str2int;
    auto fld = csv.field(fldInfo.exampleFieldIdx);
    auto it = map.find(fld);
    if (it == map.end()) {
      auto interned = xtra->intern(fld);
      i32 item = static_cast<i32>(result->strings_.size());
      result->strings_.emplace_back(interned);
      result->data_.push_back(~item);
    } else {
      result->data_.push_back(it->second);
    }
  }
  return Status::Ok();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
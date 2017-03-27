//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "training_io.h"

namespace jumanpp {
namespace core {
namespace training {

Status TrainingDataReader::initialize(const spec::TrainingSpec &spec,
                                      const CoreHolder &core) {
  for (auto &tf : spec.fields) {
    auto &dicFld = core.dic().fields().at(tf.index);
    core::dic::impl::StringStorageTraversal everything{dicFld.strings.data()};
    StringPiece data;
    util::FlatMap<StringPiece, i32> str2int;
    while (everything.next(&data)) {
      str2int[data] = everything.position();
    }
    fields_.push_back(std::move(str2int));
  }
  surfaceField_ = spec.fields[spec.surfaceIdx].index;
  return Status::Ok();
}

Status TrainingDataReader::readFullExample(analysis::ExtraNodesContext *xtra,
                                           FullyAnnotatedExample *result) {
  result->data_.clear();
  result->lengths_.clear();

  while (csv_.nextLine()) {
    if (csv_.numFields() == 1 && csv_.field(0).size() == 0) {
      return Status::Ok();
    }

    codepts_.clear();
    auto surfFld = csv_.field(surfaceField_);

    JPP_RETURN_IF_ERROR(chars::preprocessRawData(surfFld, &codepts_));
    result->lengths_.push_back(codepts_.size());

    result->surface_.append(surfFld.char_begin(), surfFld.char_end());
    for (int i = 0; i < fields_.size(); ++i) {
      auto &map = fields_[i];
      auto fld = csv_.field(i);
      auto it = map.find(fld);
      if (it == map.end()) {
        auto emb = xtra->intern(fld);
        i32 item = static_cast<i32>(result->strings_.size());
        result->strings_.push_back(emb);
        result->data_.push_back(~item);
      } else {
        result->data_.push_back(it->second);
      }
    }
  }
  finished_ = true;
  return Status::Ok();
}

Status TrainingDataReader::initCsv(StringPiece data) {
  return csv_.initFromMemory(data);
}

}  // training
}  // core
}  // jumanpp
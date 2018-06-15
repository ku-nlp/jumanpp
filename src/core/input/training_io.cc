//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "training_io.h"

namespace jumanpp {
namespace core {
namespace input {

namespace {
Status readStr2IdMap(const dic::DictionaryField &fld,
                     util::FlatMap<StringPiece, i32> *map) {
  auto &s2i = *map;

  if (fld.columnType != spec::FieldType::String) {
    return JPPS_INVALID_PARAMETER
           << "training data reader allows only string-typed fields, field "
           << fld.name << " was not";
  }

  if (fld.emptyValue.size() != 0) {
    s2i[fld.emptyValue] = 0;
  }

  core::dic::impl::StringStorageTraversal everything{fld.strings};
  StringPiece data;

  while (everything.next(&data)) {
    s2i[data] = everything.position();
  }

  return Status::Ok();
}
}  // namespace

Status TrainFieldsIndex::initialize(const CoreHolder &core) {
  auto &spec = core.spec().training;
  storages_.resize(core.spec().dictionary.numStringStorage);
  for (i32 i = 0; i < spec.fields.size(); ++i) {
    auto &tf = spec.fields[i];
    auto &fldSpec = core.spec().dictionary.fields[tf.fieldIdx];
    auto dicFld = core.dic().fields().byName(fldSpec.name);
    auto &str2int = storages_[dicFld->stringStorageIdx];

    if (str2int.size() == 0) {
      // string storage was not read yet
      JPP_RETURN_IF_ERROR(readStr2IdMap(*dicFld, &str2int));
    }

    fields_.push_back({dicFld->name, &str2int, tf.dicIdx, i});
  }
  surfaceFieldIdx_ = spec.fields[spec.surfaceIdx].number;
  return Status::Ok();
}

}  // namespace input
}  // namespace core
}  // namespace jumanpp
//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core.h"

namespace jumanpp {
namespace core {

CoreHolder::CoreHolder(const CoreConfig &conf, const RuntimeInfo &runtime,
                       const dic::DictionaryHolder &dic)
    : cfg_{conf}, runtime_{runtime}, dic_{dic} {}

Status CoreHolder::initialize() {
  JPP_RETURN_IF_ERROR(analysis::makeMakers(*this, runtime_.unkMakers, &unkMakers_));
  return Status::Ok();
}

}  // core
}  // jumanpp

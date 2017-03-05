//
// Created by Arseny Tolmachev on 2017/03/02.
//

#include "core.h"

namespace jumanpp {
namespace core {

CoreHolder::CoreHolder(const CoreConfig &conf, const RuntimeInfo &runtime,
                       const dic::DictionaryHolder &dic)
    : cfg_{conf}, runtime_{runtime}, dic_{dic} {}

}  // core
}  // jumanpp

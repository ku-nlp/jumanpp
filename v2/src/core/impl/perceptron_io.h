//
// Created by Arseny Tolmachev on 2017/05/08.
//

#ifndef JUMANPP_PERCEPTRON_IO_H
#define JUMANPP_PERCEPTRON_IO_H

#include "model_format.h"

namespace jumanpp {
namespace core {

struct PerceptronInfo {
  i32 modelSizeExponent;
};

template <typename Arch>
void Serialize(Arch& arch, PerceptronInfo& obj) {
  arch & obj.modelSizeExponent;
}

} // core
} // jumanpp


#endif //JUMANPP_PERCEPTRON_IO_H

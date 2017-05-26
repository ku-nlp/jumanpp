//
// Created by Arseny Tolmachev on 2017/05/26.
//

#ifndef JUMANPP_CG_1_SPEC_H
#define JUMANPP_CG_1_SPEC_H

#include "core/spec/spec_dsl.h"

namespace jumanpp {
namespace codegentest {

class CgOneSpecFactory {
public:
  static void fillSpec(core::spec::dsl::ModelSpecBuilder& msb) {
    auto& f1 = msb.field(1, "f1").strings().trieIndex();
    auto& f2 = msb.field(2, "f2").strings();

    msb.unigram({f1});
    msb.bigram({f1}, {f2});
    msb.bigram({f1, f2}, {f1, f2});
    msb.trigram({f1, f2}, {f1, f2}, {f1, f2});
    msb.trigram({f1, f2}, {f2}, {f1, f2});
    msb.trigram({f1}, {f1}, {f1, f2});
    msb.trigram({f2}, {f2}, {f1, f2});
  }
};

} // codegentest
} // jumanpp

#endif //JUMANPP_CG_1_SPEC_H

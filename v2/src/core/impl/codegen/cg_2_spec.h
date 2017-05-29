//
// Created by Arseny Tolmachev on 2017/05/29.
//

#ifndef JUMANPP_CG_2_SPEC_H_H
#define JUMANPP_CG_2_SPEC_H_H

#include "core/spec/spec_dsl.h"

namespace jumanpp {
namespace codegentest {

class CgTwoSpecFactory {
public:
  static void fillSpec(core::spec::dsl::ModelSpecBuilder& msb) {
    auto& f1 = msb.field(1, "f1").strings().trieIndex();
    auto& f2 = msb.field(2, "f2").strings();
    auto& f3 = msb.field(3, "f3").strings();

    msb.unigram({f1});
    msb.unigram({f2});
    msb.unigram({f3});

    msb.bigram({f1}, {f2});
    msb.bigram({f1}, {f1, f2});
    msb.bigram({f1}, {f1, f2, f3});

    msb.bigram({f1, f2}, {f1});
    msb.bigram({f1, f2}, {f1, f2});

    msb.bigram({f1, f2, f3}, {f1});

    msb.trigram({f1, f2}, {f1, f2}, {f1, f2});
    msb.trigram({f1, f2}, {f2}, {f1, f2});
    msb.trigram({f1}, {f1}, {f1, f2});
    msb.trigram({f2}, {f2}, {f1, f2});
  }
};

} // codegentest
} // jumanpp

#endif //JUMANPP_CG_2_SPEC_H_H

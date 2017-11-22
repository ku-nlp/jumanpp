//
// Created by Arseny Tolmachev on 2017/11/10.
//

#ifndef JUMANPP_SPEC_01_H
#define JUMANPP_SPEC_01_H

//
// Created by Arseny Tolmachev on 2017/05/26.
//

#include "core/spec/spec_dsl.h"

namespace jumanpp {
namespace codegentest {

class CgOneSpecFactory {
 public:
  static void fillSpec(core::spec::dsl::ModelSpecBuilder& msb) {
    auto& f1 = msb.field(1, "f1").strings().trieIndex();
    auto& f2 = msb.field(2, "f2").strings();
    auto& f3 = msb.field(3, "f3").strings();
    auto& f4 = msb.field(4, "f4").strings();
    auto& f5 = msb.field(5, "f5").strings();
    auto& f6 = msb.field(6, "f6").strings();
    auto& f7 = msb.field(7, "f7").strings();

    msb.unigram({f1});
    msb.unigram({f2});
    msb.unigram({f3});
    msb.unigram({f4});
    msb.unigram({f5, f6});
    msb.unigram({f1, f4, f5});
    msb.unigram({f1, f4, f6});
    msb.unigram({f1, f4, f7});

    msb.bigram({f1}, {f2});
    msb.bigram({f1}, {f1, f2});
    msb.bigram({f1}, {f1, f2, f3});
    msb.bigram({f1}, {f1, f2, f3, f4});
    msb.bigram({f1}, {f1, f2, f3, f4, f5});

    msb.bigram({f1, f2}, {f1});
    msb.bigram({f1, f2}, {f1, f2});
    msb.bigram({f1, f2}, {f1, f2, f3});
    msb.bigram({f1, f2}, {f1, f2, f3, f4});
    msb.bigram({f1, f2}, {f1, f2, f3, f4, f5});

    msb.bigram({f1, f2, f3}, {f1});
    msb.bigram({f1, f2, f3}, {f1, f2});
    msb.bigram({f1, f2, f3}, {f1, f2, f3});
    msb.bigram({f1, f2, f3}, {f1, f2, f3, f4});
    msb.bigram({f1, f2, f3}, {f1, f2, f3, f4, f5});

    msb.bigram({f1, f2, f3, f4}, {f1});
    msb.bigram({f1, f2, f3, f4}, {f1, f2});
    msb.bigram({f1, f2, f3, f4}, {f1, f2, f3});
    msb.bigram({f1, f2, f3, f4}, {f1, f2, f3, f4});
    msb.bigram({f1, f2, f3, f4}, {f1, f2, f3, f4, f5});

    msb.bigram({f1, f2, f3, f4, f5}, {f1});
    msb.bigram({f1, f2, f3, f4, f5}, {f1, f2});
    msb.bigram({f1, f2, f3, f4, f5}, {f1, f2, f3});
    msb.bigram({f1, f2, f3, f4, f5}, {f1, f2, f3, f4});
    msb.bigram({f1, f2, f3, f4, f5}, {f1, f2, f3, f4, f5});

    msb.trigram({f1, f2}, {f1, f2}, {f1, f2});
    msb.trigram({f1, f2}, {f2}, {f1, f2});
    msb.trigram({f1}, {f1}, {f1, f2});
    msb.trigram({f2}, {f2}, {f1, f2});
    msb.trigram({f2, f5}, {f2}, {f1, f2});
    msb.trigram({f2, f5}, {f2, f7}, {f1, f2});
    msb.trigram({f2, f5}, {f5, f7}, {f1, f2, f4});
  }
};

}  // namespace codegentest
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_01_H

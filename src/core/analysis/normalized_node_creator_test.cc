//
// Created by Arseny Tolmachev on 2017/11/23.
//

#include "testing/test_analyzer.h"
#include "util/logging.hpp"
#include "charlattice.h"

using namespace jumanpp;
using namespace jumanpp::core;
using namespace jumanpp::testing;

namespace {
class NNodeTestEnv {
  TestEnv tenv;
  StringField fld1, fld2;

  dic::DictionaryEntries dic{tenv.dic.entries()};

public:
  NNodeTestEnv(StringPiece csvData) {
    tenv.spec([](spec::dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "f1").strings().trieIndex();
      auto& f2 = specBldr.field(2, "f2").strings();
      auto& ph = specBldr.feature("ph").placeholder();
      specBldr.unk("normalize", 1).normalize().outputTo({a}).writeFeatureTo(ph);
      specBldr.unigram({a, f2, ph});
    });
    CHECK(tenv.originalSpec.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("f1", &fld1));
    REQUIRE_OK(tenv.analyzer->output().stringField("f2", &fld2));
  }

  size_t numNodeSeeds() const {
    return tenv.analyzer->latticeBuilder().seeds().size();
  }

  void printAll() {
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    auto seeds = tenv.analyzer->latticeBuilder().seeds();

    for (auto& seed : seeds) {
      CHECK(output.locate(seed.entryPtr, &walker));
      while (walker.next()) {
        LOG_TRACE() << "NODE(" << seed.codepointStart << ","
                    << seed.codepointEnd << "):" << fld1[walker] << "||"
                    << fld2[walker];
      }
    }
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->prepareNodeSeeds());
    CHECK_OK(tenv.analyzer->buildLattice());
    CHECK_OK(tenv.analyzer->bootstrapAnalysis());
  }

  charlattice::Modifiers contains(StringPiece str, i32 start, StringPiece strb) {
    CAPTURE(str);
    CAPTURE(start);
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();

    auto seeds = tenv.analyzer->latticeBuilder().seeds();
    std::vector<chars::InputCodepoint> cp;
    chars::preprocessRawData(str, &cp);
    i32 end = start + cp.size();
    for (auto& seed : seeds) {
      if (seed.codepointStart == start && seed.codepointEnd == end) {
        CHECK(output.locate(seed.entryPtr, &walker));
        while (walker.next()) {
          auto s1 = fld1[walker];
          auto s2 = fld2[walker];
          if (s1 == str && s2 == strb && seed.entryPtr.isSpecial()) {
            auto val = tenv.analyzer->extraNodesContext()->placeholderData(seed.entryPtr, 0);
            return static_cast<charlattice::Modifiers>(val);
          }
        }
      }
    }
    return charlattice::Modifiers::EMPTY;
  }

  ~NNodeTestEnv() {
    if (!Catch::getResultCapture().lastAssertionPassed()) {
      printAll();
    }
  }
};
}  // namespace

using m = charlattice::Modifiers;
using charlattice::ExistFlag;

TEST_CASE("can extract modification from normalized") {
  NNodeTestEnv env{"UNK,b\nまあ,a\n"};
  env.analyze("まあ〜");
  auto v = env.contains("まあ〜", 0, "a");
  CHECK(ExistFlag(v, m::DELETE_PROLONG));
}

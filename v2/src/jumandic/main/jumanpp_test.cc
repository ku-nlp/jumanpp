//
// Created by Arseny Tolmachev on 2017/03/10.
//

#include "jumanpp.h"
#include "util/mmap.h"
#include "testing/test_analyzer.h"

using namespace jumanpp;
using namespace jumanpp::core;

class JumandicEnv: protected JumanppExec {
protected:
  jumanpp::testing::TestEnv tenv;
  analysis::AnalysisPath top1;
public:
  JumandicEnv() {
    tenv.spec([](spec::dsl::ModelSpecBuilder& bldr) {
      jumanpp::jumandic::SpecFactory::fillSpec(bldr);
    });
    util::MappedFile fl;
    REQUIRE_OK(fl.open("jumandic/jumanpp_minimal.mdic", util::MMapType::ReadOnly));
    util::MappedFileFragment frag;
    REQUIRE_OK(fl.map(&frag, 0, fl.size()));
    tenv.saveDic(frag.asStringPiece(), "minimal.dic");
    REQUIRE_OK(init(tenv.tmpFile.name()));
  }

  void testAnalyze(StringPiece s) {
    CAPTURE(s);
    REQUIRE_OK(JumanppExec::analyze(s));
    REQUIRE_OK(top1.fillIn(analyzer.impl()->lattice()));
  }
};

TEST_CASE_METHOD(JumandicEnv, "can analyze a small sentece") {
  testAnalyze("私の物");
  WARN(format.result());
}
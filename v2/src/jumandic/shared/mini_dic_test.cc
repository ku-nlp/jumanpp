//
// Created by Arseny Tolmachev on 2017/03/08.
//

#include "testing/test_analyzer.h"
#include "jumandic_spec.h"
#include "util/mmap.h"

using namespace jumanpp::core;
using namespace jumanpp;

TEST_CASE("can import a small dictionary") {
  jumanpp::testing::TestEnv tenv;
  tenv.spec([](spec::dsl::ModelSpecBuilder& bldr) {
    jumanpp::jumandic::SpecFactory::fillSpec(bldr);
  });
  util::MappedFile fl;
  REQUIRE_OK(fl.open("jumandic/jumanpp_minimal.mdic", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(fl.map(&frag, 0, fl.size()));
  tenv.importDic(frag.asStringPiece(), "minimal.dic");
}
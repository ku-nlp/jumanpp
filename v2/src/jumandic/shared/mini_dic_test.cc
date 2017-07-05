//
// Created by Arseny Tolmachev on 2017/03/08.
//

#include "jumandic_spec.h"
#include "testing/test_analyzer.h"
#include "util/mmap.h"
#include "jumandic_id_resolver.h"

using namespace jumanpp::core;
using namespace jumanpp;

TEST_CASE("can import a small dictionary") {
  jumanpp::testing::TestEnv tenv;
  tenv.spec([](spec::dsl::ModelSpecBuilder& bldr) {
    jumanpp::jumandic::SpecFactory::fillSpec(bldr);
  });
  util::MappedFile fl;
  REQUIRE_OK(
      fl.open("jumandic/jumanpp_minimal.mdic", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(fl.map(&frag, 0, fl.size()));
  tenv.importDic(frag.asStringPiece(), "minimal.dic");
}

TEST_CASE("can build id mapping") {
  jumanpp::testing::TestEnv tenv;
  tenv.spec([](spec::dsl::ModelSpecBuilder& bldr) {
    jumanpp::jumandic::SpecFactory::fillSpec(bldr);
  });
  util::MappedFile fl;
  REQUIRE_OK(
      fl.open("jumandic/jumanpp_minimal.mdic", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(fl.map(&frag, 0, fl.size()));
  tenv.importDic(frag.asStringPiece(), "minimal.dic");
  jumanpp::jumandic::JumandicIdResolver idres;
  REQUIRE_OK(idres.initialize(tenv.core->dic()));

  util::FlatMap<StringPiece, i32> s2i;
  dic::impl::StringStorageTraversal sst{tenv.core->dic().fieldByName("pos")->strings.data()};
  StringPiece sp;
  while (sst.next(&sp)) {
    s2i[sp] = sst.position();
  }

  CHECK(s2i["名詞"] != 0);
  jumandic::JumandicPosId noun {s2i["名詞"], 0, 0, 0};
  auto jdicNoun = idres.dicToJuman(noun);
  CHECK(jdicNoun.pos == 6);
}
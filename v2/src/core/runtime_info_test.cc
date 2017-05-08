//
// Created by Arseny Tolmachev on 2017/05/08.
//

#include "runtime_info.h"
#include "core/spec/spec_dsl.h"
#include "dic_builder.h"
#include "dictionary.h"
#include "testing/standalone_test.h"

using namespace jumanpp;
using namespace jumanpp::core;

TEST_CASE("unk runtime info has correct priorities") {
  SECTION("when there is only one unk") {
    spec::dsl::ModelSpecBuilder msb;
    auto a = msb.field(1, "a").strings().trieIndex();
    msb.unk("u1", 1).chunking(chars::CharacterClass::HIRAGANA).outputTo({a});
    spec::AnalysisSpec spec{};
    REQUIRE_OK(msb.build(&spec));
    dic::DictionaryBuilder db;
    REQUIRE_OK(db.importSpec(&spec));
    REQUIRE_OK(db.importCsv("test", "a\nb\nc\nd\n"));
    dic::DictionaryHolder dh;
    REQUIRE_OK(dh.load(db.result()));
    RuntimeInfo ri;
    REQUIRE_OK(dh.compileRuntimeInfo(spec, &ri));
    CHECK(ri.unkMakers.makers.size() == 1);
    CHECK(ri.unkMakers.makers[0].priority == 0);
  }

  SECTION("when there is only one low prio unk") {
    spec::dsl::ModelSpecBuilder msb;
    auto a = msb.field(1, "a").strings().trieIndex();
    msb.unk("u1", 1)
        .chunking(chars::CharacterClass::HIRAGANA)
        .outputTo({a})
        .lowPriority();
    spec::AnalysisSpec spec{};
    REQUIRE_OK(msb.build(&spec));
    dic::DictionaryBuilder db;
    REQUIRE_OK(db.importSpec(&spec));
    REQUIRE_OK(db.importCsv("test", "a\nb\nc\nd\n"));
    dic::DictionaryHolder dh;
    REQUIRE_OK(dh.load(db.result()));
    RuntimeInfo ri;
    REQUIRE_OK(dh.compileRuntimeInfo(spec, &ri));
    CHECK(ri.unkMakers.makers.size() == 1);
    CHECK(ri.unkMakers.makers[0].priority == 1);
  }
}
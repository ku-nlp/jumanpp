//
// Created by Arseny Tolmachev on 2017/11/30.
//

#include "model_io.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::model;

TEST_CASE("saved part still has the comment") {
  ModelInfo mi;
  mi.parts.emplace_back();
  auto& p = mi.parts.back();
  p.comment = "test";
  p.kind = ModelPartKind::ScwDump;

  ModelSaver s;
  TempFile tf;
  REQUIRE(s.open(tf.name()));
  REQUIRE(s.save(mi));

  ModelInfo mi2;
  FilesystemModel fsmodel;
  REQUIRE(fsmodel.open(tf.name()));
  REQUIRE(fsmodel.load(&mi2));

  REQUIRE(mi2.parts.size() == 1);
  auto part = mi2.firstPartOf(ModelPartKind::ScwDump);
  REQUIRE(part);
  CHECK(part->comment == "test");
}
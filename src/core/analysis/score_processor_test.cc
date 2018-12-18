//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "score_processor.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp;

ConnectionBeamElement elem(u16 p1, u16 p2, Score s) { return {{p1, p2, 0}, s}; }

bool findScore(util::ArraySlice<ConnectionBeamElement> data, Score s) {
  auto it = std::find_if(
      data.begin(), data.end(),
      [&](const ConnectionBeamElement& e) { return e.totalScore == s; });
  return it != data.end();
}

TEST_CASE("fake beam is fake") { CHECK(EntryBeam::isFake(EntryBeam::fake())); }

TEST_CASE("entry beam has highest scored element") {
  std::vector<ConnectionBeamElement> elems;
  elems.resize(5);
  EntryBeam::initializeBlock(&elems);
  EntryBeam beam{&elems};
  beam.pushItem(elem(1, 2, 1.0f));
  beam.pushItem(elem(1, 2, 1.1f));
  beam.pushItem(elem(1, 2, 1.2f));
  beam.pushItem(elem(1, 2, 1.3f));
  beam.pushItem(elem(1, 2, 1.1f));
  beam.pushItem(elem(1, 2, 2));
  beam.pushItem(elem(1, 2, 1.4f));
  beam.pushItem(elem(1, 2, 2.5f));
  beam.pushItem(elem(1, 2, 1.1f));
  beam.pushItem(elem(1, 2, 1.1f));
  beam.pushItem(elem(1, 2, 1.1f));
  beam.pushItem(elem(1, 2, 1.1f));
  CHECK(findScore(elems, 2.5f));
  CHECK(findScore(elems, 2));
  CHECK(findScore(elems, 1.3f));
  CHECK(findScore(elems, 1.2f));
  CHECK(findScore(elems, 1.4f));
  CHECK_FALSE(findScore(elems, 1.0f));
  CHECK_FALSE(findScore(elems, 1.1f));
}

TEST_CASE("beam candidate is valid") {
  BeamCandidate bc{-1.0f, 5, 15};
  CHECK(bc.score() == -1.0f);
  CHECK(bc.left() == 5);
  CHECK(bc.beam() == 15);
  BeamCandidate bc2{1.5f, 55, 155};
  CHECK(bc2.score() == 1.5f);
  CHECK(bc2.left() == 55);
  CHECK(bc2.beam() == 155);
  BeamCandidate bc0{0, 0, 0};
  CHECK(bc0.score() == 0);
  CHECK(bc0.left() == 0);
  CHECK(bc0.beam() == 0);
}

TEST_CASE("beam candidates are compared corectly by score") {
  BeamCandidate bc0{0, 0, 0};
  BeamCandidate bc{-1.5f, 5, 15};
  BeamCandidate bc2{-0.1f, 5, 15};
  BeamCandidate bc3{1.5f, 55, 155};
  BeamCandidate bc4{2.5f, 556, 155};
  CHECK(bc < bc2);
  CHECK(bc < bc3);
  CHECK(bc < bc4);
  CHECK(bc2 < bc3);
  CHECK(bc2 < bc4);
  CHECK(bc3 < bc4);
  CHECK(bc4 > bc3);
  CHECK(bc4 > bc2);
  CHECK(bc4 > bc);
  CHECK(bc3 > bc2);
  CHECK(bc3 > bc);
  CHECK(bc2 > bc);
  CHECK(bc < bc0);
  CHECK(bc2 < bc0);
  CHECK(bc0 < bc3);
  CHECK(bc0 < bc4);
  CHECK(bc0 > bc);
  CHECK(bc0 > bc2);
  CHECK(bc3 > bc0);
  CHECK(bc4 > bc0);
}

TEST_CASE("beam candidates are compared correctly by coordinates") {
  BeamCandidate bc0(0.836f, 0, 0);
  BeamCandidate bc1(0.836f, 0, 2);
  BeamCandidate bc2(0.836f, 0, 3);
  BeamCandidate bc3(0.836f, 1, 0);
  BeamCandidate bc4(0.836f, 1, 3);
  CHECK(bc0 < bc1);
  CHECK(bc0 < bc2);
  CHECK(bc0 < bc3);
  CHECK(bc0 < bc4);
  CHECK(bc1 > bc0);
  CHECK(bc1 < bc2);
  CHECK(bc1 < bc3);
  CHECK(bc1 < bc4);
  CHECK(bc2 > bc0);
  CHECK(bc2 > bc1);
  CHECK(bc2 < bc3);
  CHECK(bc2 < bc4);
  CHECK(bc3 > bc0);
  CHECK(bc3 > bc1);
  CHECK(bc3 > bc2);
  CHECK(bc3 < bc4);
}
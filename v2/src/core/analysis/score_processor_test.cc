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
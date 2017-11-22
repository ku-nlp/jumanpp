//
// Created by Arseny Tolmachev on 2017/02/26.
//

#include "testing/standalone_test.h"

#include <util/array_slice.h>
#include <util/types.hpp>
#include "testing/standalone_test.h"
#include "util/memory.hpp"
#include "util/soa.h"

using namespace jumanpp;
using namespace jumanpp::util;
using namespace jumanpp::util::memory;

struct BeamNodeReference {
  float score;
};

class LatticeBoundaryContainer : public StructOfArrays {
  SizedArrayField<BeamNodeReference> beams;
  SizedArrayField<u64> featurePatterns;

 public:
  LatticeBoundaryContainer(util::memory::PoolAlloc* alloc, u16 beam,
                           u16 patternCnt, u16 numNodes)
      : StructOfArrays(alloc, numNodes),
        beams{this, static_cast<size_t>(beam)},
        featurePatterns{this, static_cast<size_t>(patternCnt)} {}
};

class BoundaryConnection final
    : public StructOfArraysFactory<BoundaryConnection> {
  SizedArrayField<i32> ptrs;
  SizedArrayField<float, 64> vectors;

 public:
  BoundaryConnection(PoolAlloc* alloc, u32 leftCount, u32 rightCount,
                     u32 beamSize)
      : StructOfArraysFactory(alloc, beamSize * rightCount, leftCount),
        ptrs{this, 10},
        vectors{this, 96} {}

  BoundaryConnection(const BoundaryConnection& o)
      : StructOfArraysFactory(o), ptrs{this, 10}, vectors{this, 96} {}

  MutableArraySlice<i32> ptrsData() { return ptrs.data(); }
};

TEST_CASE("lattice container can be created") {
  util::memory::Manager mgr(4 * 1024 * 1024);  // 4M
  auto core = mgr.core();
  LatticeBoundaryContainer lbc{core.get(), 6, 10, 15};
  CHECK_OK(lbc.initialize());
}

TEST_CASE("boundary connection") {
  util::memory::Manager mgr{2 * 1024 * 1024};
  auto core = mgr.core();
  BoundaryConnection cnn{core.get(), 20, 20, 5};
  auto& c1 = cnn.makeOne();
  auto& c2 = cnn.makeOne();
  CHECK(c2.ptrsData().data() != nullptr);
  CHECK(c2.ptrsData().size() == 20 * 10 * 5);
}
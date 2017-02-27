//
// Created by Arseny Tolmachev on 2017/02/24.
//

#include "testing/standalone_test.h"
#include "lattice_types.h"

using namespace jumanpp::core::analysis;
namespace m = jumanpp::util::memory;

TEST_CASE("lattice can be created") {
  m::Manager mgr{4*1024*1024};
  auto core = mgr.core();
  LatticeConfig lc {4, 2, 2, 2, 3};
  Lattice l{core.get(), lc};
}
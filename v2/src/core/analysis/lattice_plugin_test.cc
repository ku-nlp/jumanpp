//
// Created by Arseny Tolmachev on 2017/03/07.
//

#include "lattice_plugin.h"
#include "lattice_types.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::util::memory;

class TestPlugin1 : public LatticePlugin {
 public:
  LazyField<SizedArrayField<float>> floats;
  LazyField<SizedArrayField<float>> floats2;

  TestPlugin1() {}
  ~TestPlugin1() override {}

  LatticePlugin *clone(ManagedAllocatorCore *alloc) const override {
    return alloc->make<TestPlugin1>();
  }

  void install(LatticeRightBoundary *right) override {
    floats.initialize(right, 100u);
  }

  void install(LatticeBoundaryConnection *connection) override {
    floats2.initialize(connection, 100u);
  }
};

TEST_CASE("LatticePlugin is useful") {
  Manager mgr{1024 * 1024};
  auto alloc = mgr.core();
  LatticeConfig lconf{10, 5, 4, 3, 2};
  Lattice l{alloc.get(), lconf};
  TestPlugin1 tp;
  l.installPlugin(&tp);
  LatticeBoundaryConfig lbc{0, 10, 10};
  LatticeBoundary *bnd;
  REQUIRE_OK(l.makeBoundary(lbc, &bnd));
  auto tp1c = bnd->starts()->plugin<TestPlugin1>();
  CHECK(tp1c != nullptr);
  CHECK(tp1c->floats.value().data().size() == 10 * 100);
}
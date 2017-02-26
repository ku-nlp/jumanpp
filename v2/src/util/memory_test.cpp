//
// Created by Arseny Tolmachev on 2017/02/16.
//

#include <testing/standalone_test.h>

#include <iostream>
#include <util/logging.hpp>
#include <util/memory.hpp>

namespace m = jumanpp::util::memory;

TEST_CASE("powers of 2") {
  CHECK(m::IsPowerOf2(0));
  CHECK(m::IsPowerOf2(1));
  CHECK(m::IsPowerOf2(2));
  CHECK(m::IsPowerOf2(4));
  CHECK(m::IsPowerOf2(8));
  CHECK(m::IsPowerOf2(16));
  CHECK(m::IsPowerOf2(32));
  CHECK(m::IsPowerOf2(64));
  CHECK(m::IsPowerOf2(128));

  CHECK_FALSE(m::IsPowerOf2(3));
  CHECK_FALSE(m::IsPowerOf2(5));
  CHECK_FALSE(m::IsPowerOf2(9));
  CHECK_FALSE(m::IsPowerOf2(17));
  CHECK_FALSE(m::IsPowerOf2(231));
}

TEST_CASE("align works") {
  CHECK(m::Align(0, 4) == 0);
  CHECK(m::Align(1, 4) == 4);
  CHECK(m::Align(2, 4) == 4);
  CHECK(m::Align(3, 4) == 4);
  CHECK(m::Align(4, 4) == 4);
  CHECK(m::Align(5, 4) == 8);

  CHECK(m::Align(0, 8) == 0);
  CHECK(m::Align(4, 8) == 8);
  CHECK(m::Align(5, 8) == 8);
  CHECK(m::Align(6, 8) == 8);
  CHECK(m::Align(7, 8) == 8);
  CHECK(m::Align(8, 8) == 8);
  CHECK(m::Align(9, 8) == 16);
}

TEST_CASE("allocator works with several pages") {
  m::Manager mgr{1024};
  auto c = mgr.core();
  auto a1 = c->allocateArray<int>(200);
  auto a2 = c->allocateArray<int>(200);
  auto a3 = c->allocateArray<int>(200);
  auto a4 = c->allocateArray<int>(200);
  CHECK(a1 != a2);
  CHECK(a1 != a3);
  CHECK(a3 != a4);
}

TEST_CASE("allocator works with several pages and reset") {
  m::Manager mgr{1024};
  auto c = mgr.core();
  auto a1 = c->allocateArray<int>(200);
  auto a2 = c->allocateArray<int>(200);
  auto a3 = c->allocateArray<int>(200);
  auto a4 = c->allocateArray<int>(200);
  CHECK(a1 != a2);
  CHECK(a1 != a3);
  CHECK(a3 != a4);

  mgr.reset();

  auto b1 = c->allocateArray<int>(180);
  auto b2 = c->allocateArray<int>(180);
  auto b3 = c->allocateArray<int>(180);
  auto b4 = c->allocateArray<int>(180);
  CHECK(b1 != b2);
  CHECK(b2 != b3);
  CHECK(b3 != b4);
  CHECK(b1 != b3);
  CHECK(a1 == b1);
  CHECK(a2 == b2);
  CHECK(a3 == b3);
  CHECK(a4 == b4);
}

TEST_CASE("memory is aligned") {
  m::Manager mgr {1024};
  auto c = mgr.core();
  c->allocate_memory(4, 4);
  auto ptr = c->allocate_memory(8, 8);
  CHECK(m::IsAligned(reinterpret_cast<size_t>(ptr), 8));
  auto ptr2 = c->allocate_memory(8, 128);
  CHECK(m::IsAligned(reinterpret_cast<size_t>(ptr2), 128));
}

TEST_CASE("StlManagedAlloc adheres to stl spec") {
  m::Manager mgr_{1024 * 1024};
  auto core1 = mgr_.core();
  auto core2 = mgr_.core();

  m::StlManagedAlloc<int> a1{core1.get()};
  m::StlManagedAlloc<int> a2{core2.get()};
  CHECK(a1 != a2);

  m::StlManagedAlloc<int> a3{a2};
  CHECK(a2 == a3);
  CHECK(a1 != a3);

  m::StlManagedAlloc<int> a4{std::move(a3)};
  CHECK(a2 == a4);
  CHECK(a1 != a4);
}

TEST_CASE("std::vector works with managedalloc") {
  m::Manager mgr_{1024};
  auto core = mgr_.core();
  m::ManagedVector<int> mvec{core.get()};
  mvec.push_back(1);
  mvec.push_back(2);
  mvec.push_back(3);
  CHECK(mvec.size() == 3);
  CHECK(mvec.back() == 3);
}
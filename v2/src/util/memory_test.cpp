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

TEST_CASE("basic allocator is working", "[memory]") {
  m::Manager mgr{1 << 10};
  auto alloc = mgr.allocator<int>();
  auto intptr = alloc.make(1);
  CHECK(intptr != nullptr);
  CHECK(*intptr == 1);
}

TEST_CASE("StlManagedAlloc adheres to stl spec") {
  m::Manager mgr_{1024*1024};
  auto core1 = mgr_.core();
  auto core2 = mgr_.core();

  m::StlManagedAlloc<int> a1{&core1};
  m::StlManagedAlloc<int> a2{&core2};
  CHECK(a1 != a2);

  m::StlManagedAlloc<int> a3{a2};
  CHECK(a2 == a3);
  CHECK(a1 != a3);

  m::StlManagedAlloc<int> a4{std::move(a3)};
  CHECK(a2 == a4);
  CHECK(a1 != a4);
}

TEST_CASE("std::vector works with managedalloc") {
  m::Manager mgr_{1024*1024};
  auto core = mgr_.core();
  std::vector<int, m::StlManagedAlloc<int>> mvec{&core};
  mvec.push_back(1);
  mvec.push_back(2);
  mvec.push_back(3);
  CHECK(mvec.size() == 3);
  CHECK(mvec.back() == 3);
}
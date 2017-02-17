//
// Created by Arseny Tolmachev on 2017/02/16.
//

#include <catch.hpp>

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

TEST_CASE("basic allocator is working", "[memory]") {
  m::Manager mgr{1 << 10};
  auto alloc = mgr.allocator<int>();
  auto intptr = alloc.make(1);
  CHECK(intptr != nullptr);
  CHECK(*intptr == 1);
}
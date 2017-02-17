//
// Created by Arseny Tolmachev on 2017/02/15.
//
#include <catch.hpp>

#include <iostream>
#include <util/status.hpp>

using namespace jumanpp;

Status makeStatus() { return Status::InvalidParameter() << "test"; }

TEST_CASE("Status constructor works", "[status]") {
  Status s = makeStatus();
  CHECK(s.message == "test");
  CHECK(s.code == StatusCode::InvalidParameter);
}
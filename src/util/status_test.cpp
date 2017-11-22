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
  CHECK(s.message() == "test");
  CHECK(s.code() == StatusCode::InvalidParameter);
}

std::string statusString(const Status &s) {
  std::stringstream ss;
  ss << s;
  return ss.str();
}

TEST_CASE("ok has empty string representation") {
  CHECK(statusString(Status::Ok()) == "");
}

TEST_CASE("invalidparameter has correct string representation") {
  CHECK(statusString(Status::InvalidParameter()) == "InvalidParameter");
}

TEST_CASE("invalidstate has correct string representation") {
  CHECK(statusString(Status::InvalidState()) == "InvalidState");
}

TEST_CASE("notimplemented has correct string representation") {
  CHECK(statusString(Status::NotImplemented()) == "NotImplemented");
}

Status badOp() { return JPPS_NOT_IMPLEMENTED << "fail"; }

Status filter1() {
  JPP_RETURN_IF_ERROR(badOp());
  return Status::Ok();
}

Status filter2() {
  JPP_RIE_MSG(badOp(), "help" << 2);
  return Status::Ok();
}

TEST_CASE("status constructor with frame works") {
  Status s = badOp();
  auto string = statusString(s);
  CHECK_THAT(string, Catch::Contains("backtrace:"));
  CHECK_THAT(string, Catch::Contains("badOp"));
}

TEST_CASE("status constructor with two frames work") {
  Status s = filter1();
  auto string = statusString(s);
  CHECK_THAT(string, Catch::Contains("backtrace:"));
  CHECK_THAT(string, Catch::Contains("badOp"));
  CHECK_THAT(string, Catch::Contains("filter1"));
}

TEST_CASE("status constructor with two frames and message work") {
  Status s = filter2();
  auto string = statusString(s);
  CHECK_THAT(string, Catch::Contains("backtrace:"));
  CHECK_THAT(string, Catch::Contains("badOp"));
  CHECK_THAT(string, Catch::Contains("filter2"));
  CHECK_THAT(string, Catch::Contains("help2"));
}
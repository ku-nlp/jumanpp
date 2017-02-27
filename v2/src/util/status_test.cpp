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

std::string statusString(Status s) {
  std::stringstream ss;
  ss << s;
  return ss.str();
}

TEST_CASE("ok has correct string representation") {
   CHECK(statusString(Status::Ok()) == "0:Ok");
}

TEST_CASE("invalidparameter has correct string representation") {
  CHECK(statusString(Status::InvalidParameter()) == "1:InvalidParameter");
}

TEST_CASE("invalidstate has correct string representation") {
  CHECK(statusString(Status::InvalidState()) == "2:InvalidState");
}

TEST_CASE("notimplemented has correct string representation") {
  CHECK(statusString(Status::NotImplemented()) == "3:NotImplemented");
}

TEST_CASE("endofiteration has correct string representation") {
  CHECK(statusString(Status::EndOfIteration()) == "4:EndOfIteration");
}

TEST_CASE("maxstatus has correct string representation") {
  Status s{StatusCode::MaxStatus};
  int maxStatusCode = static_cast<int>(StatusCode::MaxStatus);
  std::stringstream ss;
  ss << maxStatusCode << ":MaxStatus";
  CHECK(statusString(s) == ss.str());
}
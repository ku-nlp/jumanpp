//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "coded_io.h"
#include <testing/standalone_test.h>

namespace u = jumanpp::util;
using jumanpp::u64;

TEST_CASE("coded io works there and back with ints") {
  u::CodedBuffer out;
  out.writeVarint(1);
  out.writeVarint(2);
  out.writeVarint(32451231);
  u::CodedBufferParser p;
  p.reset(out.contents());
  CHECK(p.remaining() == 6);
  u64 ret = 0;
  CHECK(p.readVarint64(&ret));
  CHECK(ret == 1);
  CHECK(p.readVarint64(&ret));
  CHECK(ret == 2);
  CHECK(p.readVarint64(&ret));
  CHECK(ret == 32451231);
  CHECK(p.remaining() == 0);
  CHECK_FALSE(p.readVarint64(&ret));
}

TEST_CASE("coded parser doesn't crash when empty") {
  u::CodedBufferParser p;
  u64 x;
  CHECK_FALSE(p.readVarint64(&x));
  jumanpp::StringPiece sp;
  CHECK_FALSE(p.readStringPiece(&sp));
}

TEST_CASE("coded io works with two strings") {
  u::CodedBuffer buf;
  buf.writeString("test");
  buf.writeString("help");
  u::CodedBufferParser p;
  p.reset(buf.contents());
  jumanpp::StringPiece sp;
  CHECK(p.readStringPiece(&sp));
  CHECK(sp == "test");
  CHECK(p.readStringPiece(&sp));
  CHECK(sp == "help");
  CHECK_FALSE(p.readStringPiece(&sp));
}

TEST_CASE("coded io works with string and int") {
  u::CodedBuffer buf;
  buf.writeVarint(48645);
  buf.writeString("test");
  buf.writeVarint(412);
  u::CodedBufferParser p;
  p.reset(buf.contents());
  jumanpp::StringPiece sp;
  u64 x = 0;
  CHECK(p.readVarint64(&x));
  CHECK(x == 48645);
  CHECK(p.readStringPiece(&sp));
  CHECK(sp == "test");
  CHECK(p.readVarint64(&x));
  CHECK(x == 412);
  CHECK_FALSE(p.readStringPiece(&sp));
  CHECK_FALSE(p.readVarint64(&x));
}

TEST_CASE("read fails when a large integer is truncated") {
  u::CodedBuffer buf;
  buf.writeVarint(382190578192);
  auto slice = buf.contents().slice(1, 5);
  u::CodedBufferParser p { slice };
  u64 x;
  CHECK_FALSE(p.readVarint64(&x));
}

TEST_CASE("read fails when string buffer is truncated") {
  u::CodedBuffer buf;
  buf.writeString("hello, world!");
  auto slice = buf.contents().slice(0, 3);
  u::CodedBufferParser p { slice };
  CHECK_FALSE(p.readStringPiece(nullptr));
}
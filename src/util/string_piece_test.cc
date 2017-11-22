//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "string_piece.h"
#include <testing/standalone_test.h>

using jumanpp::StringPiece;
using jumanpp::u8;

TEST_CASE("stringpiece can be constructed", "[string_piece]") {
  CHECK(StringPiece(std::string("test")) == "test");
  CHECK(StringPiece("test") == "test");
  CHECK(StringPiece(std::vector<char>{'t', 'e', 's', 't'}) == "test");
  CHECK_FALSE(StringPiece(std::string("test2")) == "test");
  CHECK_FALSE(StringPiece(std::string("test")) == "test2");
  CHECK_FALSE(StringPiece("test2") == "test");
  CHECK_FALSE(StringPiece("test") == "test2");
  CHECK_FALSE(StringPiece(std::vector<char>{'t', 'e', 's', 't', '2'}) ==
              "test");
  CHECK_FALSE(StringPiece(std::vector<char>{'t', 'e', 's', 't'}) == "test2");

  // implicit construction of StringPiece here
  CHECK(StringPiece("test") == std::string("test"));
}

TEST_CASE("stringpiece has working mechanisms of substringing") {
  StringPiece a{"a test string"};
  CHECK(a.slice(0, 1) == "a");
  CHECK(a.slice(2, 2 + 4) == "test");
}

TEST_CASE("stringpiece has working std::hash and std::equal_to impls") {
  std::hash<StringPiece> hashFn;
  std::equal_to<StringPiece> equalFn;

  CHECK(hashFn("test") == hashFn("test"));
  CHECK(hashFn("test5") == hashFn("test5"));
  CHECK(hashFn("test5") != hashFn("test3"));

  CHECK(equalFn("test", "test"));
  CHECK(equalFn("test5", "test5"));
  CHECK_FALSE(equalFn("test5", "test3"));
}

TEST_CASE("StringPiece can be constexpr") {
  constexpr StringPiece sp{"test"};
  CHECK(sp == "test");
}

TEST_CASE("StringPiece constructs from CString") {
  auto sp = StringPiece::fromCString("test");
  CHECK(sp == "test");
}
//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "string_piece.h"
#include <testing/standalone_test.h>

using jumanpp::StringPiece;

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
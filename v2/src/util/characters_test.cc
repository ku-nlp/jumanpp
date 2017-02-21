

#include "characters.hpp"
#include <testing/standalone_test.h>

using jumanpp::StringPiece;
using namespace jumanpp::chars;

TEST_CASE("toCodepoint works with あ", "[string_piece]") {
    char32_t result;    
    auto literal = "あいうえお";
    CHECK(toCodepoint(literal, literal+sizeof(literal)-1, &result));
}

TEST_CASE("テストケース名", "[string_piece]") {

  //Status preprocessRawData(StringPiece utf8data,
  //                       std::vector<InputCodepoint> *result) {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("", &result));
  CHECK(result.size() == 0);

  result.clear();
  CHECK_OK(preprocessRawData("test", &result));
  CHECK(result.size() == 4);
  CHECK(result[0].codepoint == U't');
  CHECK(result[3].codepoint == U't');
  
  result.clear();
  CHECK_OK(preprocessRawData("あいうえお", &result));
  CHECK(result.size() == 5);
  CHECK(result[0].codepoint == U'あ');
  CHECK(result[4].codepoint == U'お');
  
  result.clear();
  CHECK_OK(preprocessRawData("ｱｲｳｴｵ", &result));
  CHECK(result.size() == 5);
  CHECK(result[0].codepoint == U'ｱ');
  CHECK(result[4].codepoint == U'ｵ');
  
  result.clear();
  CHECK_OK(preprocessRawData("亜", &result));
  CHECK(result.size() == 1);
  CHECK(result[0].codepoint == U'亜');
  
  result.clear();
  CHECK_OK(preprocessRawData("🍣🍺", &result));
  CHECK(result.size() == 1);
  CHECK(result[0].codepoint == U'🍣');
  CHECK(result[1].codepoint == U'🍺');
}


  // Invalid UTF8 sequence
  //CHECK_FALSE(StringPiece(std::string("test2")) == "test");

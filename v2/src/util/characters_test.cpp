//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "util/characters.h"
#include "testing/standalone_test.h"

using jumanpp::StringPiece;
using namespace jumanpp::chars;
using namespace jumanpp;

bool toCodepoint(const char*& itr, const char* end, char32_t* result) noexcept {
  return toCodepoint(reinterpret_cast<StringPiece::pointer_t&>(itr),
                     reinterpret_cast<StringPiece::pointer_t>(end), result);
}

Status checkByteSequence(std::initializer_list<u8> useq) {
  std::vector<InputCodepoint> result;
  std::vector<u8> malformed_seq{useq};
  StringPiece sp(reinterpret_cast<const char*>(malformed_seq.data()),
                 reinterpret_cast<const char*>(malformed_seq.data() +
                                               malformed_seq.size()));
  return (preprocessRawData(sp, &result));
}

TEST_CASE("character classes are compatible", "[characters]") {
  CHECK(IsCompatibleCharClass(CharacterClass::SPACE,
                              CharacterClass::FAMILY_SPACE));
  CHECK(IsCompatibleCharClass(CharacterClass::HIRAGANA,
                              CharacterClass::HIRAGANA));
  CHECK(IsCompatibleCharClass(CharacterClass::KANJI,
                              CharacterClass::FAMILY_KANJI));
  CHECK(IsCompatibleCharClass(CharacterClass::ALPH,
                              CharacterClass::FAMILY_ALPH_PUNC));

  CHECK_FALSE(
      IsCompatibleCharClass(CharacterClass::KANJI, CharacterClass::ALPH));
}

TEST_CASE("toCodepoint works with あ", "[characters]") {
  char32_t result;
  auto literal = "あ";
  CHECK(toCodepoint(literal, literal + sizeof(literal) - 1, &result));
  CHECK(result == U'あ');
}

TEST_CASE("space type of codepints is detected correctly", "[characters]") {
  CHECK(getCodeType(U' ') == CharacterClass::SPACE);  // U+0020 simple space
  CHECK(getCodeType(U' ') == CharacterClass::SPACE);  //	U+00A0 NO-BREAK
                                                      // SPACE
  CHECK(getCodeType(U'　') ==
        CharacterClass::SPACE);  // U+3000 IDEOGRAPHIC SPACE (full width space)
  CHECK(getCodeType(0xFEFF) ==
        CharacterClass::SPACE);  //	ZERO WIDTH NO-BREAK SPACE
}

TEST_CASE("preprocessRawData works with blank string", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("", &result));
  CHECK(result.size() == 0);
}

TEST_CASE("preprocessRawData works with alphabets", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("test", &result));
  CHECK(result.size() == 4);
  CHECK(result[0].codepoint == U't');
  CHECK(result[0].hasClass(CharacterClass::ALPH));
  CHECK(result[3].codepoint == U't');
  CHECK(result[3].hasClass(CharacterClass::ALPH));
}

TEST_CASE("preprocessRawData works with Hiragana", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("あいうえお", &result));
  CHECK(result.size() == 5);
  CHECK(result[0].codepoint == U'あ');
  CHECK(result[0].hasClass(CharacterClass::HIRAGANA));
  CHECK(result[4].codepoint == U'お');
  CHECK(result[4].hasClass(CharacterClass::HIRAGANA));
}

TEST_CASE("preprocessRawData works with Katakana", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("アイウエオ", &result));
  CHECK(result.size() == 5);
  CHECK(result[0].codepoint == U'ア');
  CHECK(result[0].hasClass(CharacterClass::KATAKANA));
  CHECK(result[4].codepoint == U'オ');
  CHECK(result[4].hasClass(CharacterClass::KATAKANA));
}

TEST_CASE("preprocessRawData works with Half Katakanas", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("ｱｲｳｴｵ", &result));
  CHECK(result.size() == 5);
  CHECK(result[0].codepoint == U'ｱ');
  CHECK(result[0].hasClass(CharacterClass::HANKAKU_KANA));
  CHECK(result[4].codepoint == U'ｵ');
  CHECK(result[4].hasClass(CharacterClass::HANKAKU_KANA));
}

TEST_CASE("preprocessRawData works with Kanjis", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("漢字", &result));
  CHECK(result.size() == 2);
  CHECK(result[0].codepoint == U'漢');
  CHECK(result[0].hasClass(CharacterClass::KANJI));
  CHECK(result[1].codepoint == U'字');
  CHECK(result[1].hasClass(CharacterClass::KANJI));
}

TEST_CASE("preprocessRawData works with Emojis", "[characters]") {
  std::vector<InputCodepoint> result;
  CHECK_OK(preprocessRawData("🍣🍺", &result));
  CHECK(result.size() == 2);
  CHECK(result[0].codepoint == U'🍣');
  CHECK(result[0].hasClass(CharacterClass::SYMBOL));
  CHECK(result[1].codepoint == U'🍺');
  CHECK(result[1].hasClass(CharacterClass::SYMBOL));
}

TEST_CASE("compute correct length", "[characters]") {
  CHECK(numCodepoints("test") == 4);
  CHECK(numCodepoints("🍣🍺") == 2);
  CHECK(numCodepoints("漢字") == 2);
  CHECK(numCodepoints("ｲｳｴｵ") == 4);
  CHECK(numCodepoints("привет") == 6);
}

TEST_CASE("preprocessRawData collectly treats UTF8 sequence あ",
          "[characters]") {
  CHECK(checkByteSequence({0xe3, 0x81, 0x82}));
}

TEST_CASE(
    "preprocessRawData collectly treats malformed UTF8 continuation byte "
    "sequences",
    "[characters]") {
  std::vector<InputCodepoint> result;
  // First continuation byte 0x80
  CHECK_FALSE(checkByteSequence({0x80}));
  // Last continuation byte 0xbf
  CHECK_FALSE(checkByteSequence({0xbf}));
  // Last continuation byte 0xbf
  CHECK_FALSE(checkByteSequence({0xbf, 0xbf}));
  // 2 continuation bytes 0x80, 0x80
  CHECK_FALSE(checkByteSequence({0x80, 0x80}));
  // 3 continuation bytes 0x80, 0x80, 0x80
  CHECK_FALSE(checkByteSequence({0x80, 0x80, 0x80}));
  // 4 continuation bytes 0x80, 0x80, 0x80, 0x80
  CHECK_FALSE(checkByteSequence({0x80, 0x80, 0x80, 0x80}));
}

TEST_CASE("preprocessRawData collectly treats impossible utf8 byte sequences",
          "[characters]") {
  // Impossible bytes
  // 0xfe
  CHECK_FALSE(checkByteSequence({0xfe}));
  // 0xff
  CHECK_FALSE(checkByteSequence({0xff}));
  // 0xfe 0xfe 0xff 0xff
  CHECK_FALSE(checkByteSequence({0xfe, 0xfe, 0xff, 0xff}));
}
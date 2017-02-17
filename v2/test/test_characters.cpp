//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include <catch.hpp>

#include <util/characters.hpp>

using namespace jumanpp;

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

TEST_CASE("string is processed to character information", "[characters]") {}

//
// Created by Arseny Tolmachev on 2017/02/17.
//

#ifndef JUMANPP_CHARACTERS_HPP
#define JUMANPP_CHARACTERS_HPP

#include "util/status.hpp"
#include "util/string_piece.h"
#include "util/types.hpp"

#include <vector>

namespace jumanpp {
namespace chars {

enum class CharacterClass;

inline CharacterClass operator|(CharacterClass c1, CharacterClass c2) noexcept {
  return static_cast<CharacterClass>(static_cast<int>(c1) |
                                     static_cast<int>(c2));
}

inline CharacterClass operator&(CharacterClass c1, CharacterClass c2) noexcept {
  return static_cast<CharacterClass>(static_cast<int>(c1) &
                                     static_cast<int>(c2));
}

enum class CharacterClass : i32 {
  SPACE = 0x00000001,
  IDEOGRAPHIC_PUNC = 0x00000002,  // 、。
  KANJI = 0x00000004,
  FIGURE = 0x00000008,
  PERIOD = 0x00000010,      // ．
  MIDDLE_DOT = 0x00000020,  // ・
  COMMA = 0x00000040,       //　，
  ALPH = 0x00000080,
  SYMBOL = 0x00000100,
  KATAKANA = 0x00000200,  // カタカナ or 長音?
  HIRAGANA = 0x00000400,
  KANJI_FIGURE = 0x00000800,  // KANJI+FIGURE にするべき？
  SLASH = 0x00001000,
  COLON = 0x00002000,
  ERA = 0x00004000,               // ㍻
  CHOON = 0x00008000,             // ー, 〜
  HANKAKU_KANA = 0x00010000,      // 半角カタカナ
  BRACKET = 0x00020000,           // 括弧, 引用符
  FIGURE_EXCEPTION = 0x00040000,  // 数，何
  FIGURE_DIGIT = 0x00080000,      // 十，百，千，万，億
  LOWER_CASE = 0x00100000,  // ぁ，ぃ，ぅ，ァ，ィ，ゥ，っ，ッ

  FAMILY_FIGURE = FIGURE | PERIOD | MIDDLE_DOT | KANJI_FIGURE | SLASH | COLON,
  FAMILY_PUNC = PERIOD | COMMA | IDEOGRAPHIC_PUNC,
  FAMILY_ALPH_PUNC = ALPH | PERIOD | SLASH | COLON | MIDDLE_DOT,
  FAMILY_PUNC_SYMBOL =
      PERIOD | COMMA | IDEOGRAPHIC_PUNC | MIDDLE_DOT | SYMBOL | SLASH | COLON,
  FAMILY_SPACE = SPACE,
  FAMILY_SYMBOL = SYMBOL,
  FAMILY_ALPH = ALPH,
  FAMILY_KANJI = KANJI | KANJI_FIGURE,
  FAMILY_BRACKET = BRACKET,
  FAMILY_OTHERS = 0x00000000,
};

inline bool IsCompatibleCharClass(CharacterClass realCharClass,
                                  CharacterClass familyOrTarget) noexcept {
  return (realCharClass & familyOrTarget) != CharacterClass::FAMILY_OTHERS;
}

struct InputCodepoint {
  /**
   * Unicode codepoint for character
   */
  char32_t codepoint;

  CharacterClass charClass;

  /**
   * Rererence to original bytes
   */
  StringPiece bytes;

  inline bool hasClass(CharacterClass queryClass) const noexcept {
    return IsCompatibleCharClass(charClass, queryClass);
  }
};

bool toCodepoint(StringPiece::pointer_t &itr, StringPiece::pointer_t itr_end,
                 char32_t *result) noexcept;

CharacterClass getCodeType(char32_t code) noexcept;

Status preprocessRawData(StringPiece utf8data,
                         std::vector<InputCodepoint> *result);

}  // namespace chars
}  // namespace jumanpp

#endif  // JUMANPP_CHARACTERS_HPP

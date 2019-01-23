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
  SMALL_KANA = 0x00100000,  // ぁ，ぃ，ぅ，ァ，ィ，ゥ，っ，ッ

  FAMILY_FIGURE = FIGURE | PERIOD | MIDDLE_DOT | KANJI_FIGURE | SLASH | COLON,
  FAMILY_PUNC = PERIOD | COMMA | IDEOGRAPHIC_PUNC,
  FAMILY_ALPH_PUNC = ALPH | PERIOD | SLASH | COLON | MIDDLE_DOT,
  FAMILY_NUM_PERIOD = PERIOD | MIDDLE_DOT,
  FAMILY_PUNC_SYMBOL =
      PERIOD | COMMA | IDEOGRAPHIC_PUNC | MIDDLE_DOT | SYMBOL | SLASH | COLON,
  FAMILY_SPACE = SPACE,
  FAMILY_SYMBOL = SYMBOL,
  FAMILY_ALPH = ALPH,
  FAMILY_KANJI = KANJI | KANJI_FIGURE,
  FAMILY_KANA = KATAKANA | HIRAGANA | HANKAKU_KANA | SMALL_KANA,
  FAMILY_DOUBLE =
      KATAKANA | HIRAGANA | HANKAKU_KANA | SMALL_KANA | KANJI | CHOON,
  FAMILY_BRACKET = BRACKET,
  FAMILY_DIGITS = FIGURE | KANJI_FIGURE | FIGURE_DIGIT,
  FAMILY_EXCEPTION = FIGURE | KANJI_FIGURE | FIGURE_EXCEPTION,
  FAMILY_PROLONGABLE = KANJI | HIRAGANA | KATAKANA,
  FAMILY_FULL_KANA = HIRAGANA | KATAKANA,
  FAMILY_OTHERS = 0x00000000,
  FAMILY_ANYTHING = 0x7fffffff,
};

std::ostream& operator<<(std::ostream& os, CharacterClass cc);

struct CodePointInfo {
  char32_t codepoint;
  int utf8Length;
};

#define JPP_CHAR_CHECK(x)                  \
  {                                        \
    if (JPP_UNLIKELY(!(x))) return {0, 0}; \
  }

inline CodePointInfo getCodepoint(const u8* itr, const u8* itr_end) noexcept {
  char32_t u = 0;

  if (*itr > 0x00ef) { /* 4 bytes */
    JPP_CHAR_CHECK(itr_end - itr >= 4);
    JPP_CHAR_CHECK(((*itr & ~0x7) ^ 0xf0) == 0);
    u = (*(itr++) & 0x07u) << 18;
    JPP_CHAR_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3fu) << 12;
    JPP_CHAR_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3fu) << 6;
    JPP_CHAR_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3fu);
    return {u, 4};
  } else if (*itr > 0xdf) { /* 3 bytes */
    JPP_CHAR_CHECK(itr_end - itr >= 3);
    JPP_CHAR_CHECK(((*itr & ~0xf) ^ 0xe0) == 0);
    u = (*(itr++) & 0x0fu) << 12;
    JPP_CHAR_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3fu) << 6;
    JPP_CHAR_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= *(itr++) & 0x3fu;
    return {u, 3};
  } else if (*itr > 0x7f) { /* 2 bytes */
    JPP_CHAR_CHECK(itr_end - itr >= 2);
    JPP_CHAR_CHECK(((*itr & ~0x1f) ^ 0xc0) == 0);
    u = (*(itr++) & 0x1fu) << 6;
    JPP_CHAR_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= *(itr++) & 0x3fu;
    return {u, 2};
  } else { /* 1 byte */
    JPP_CHAR_CHECK(itr_end - itr >= 1);
    JPP_CHAR_CHECK((*itr & ~0x7f) == 0);
    u = *(itr++) & 0x7fu;
    return {u, 1};
  }
}

#undef JPP_CHAR_CHECK

inline bool IsCompatibleCharClass(CharacterClass realCharClass,
                                  CharacterClass familyOrTarget) noexcept {
  return (realCharClass & familyOrTarget) != CharacterClass::FAMILY_OTHERS;
}

CharacterClass getCodeType(char32_t code) noexcept;

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

  JPP_ALWAYS_INLINE constexpr InputCodepoint(const char32_t cp,
                                             const CharacterClass& cc,
                                             StringPiece b) noexcept
      : codepoint(cp), charClass{cc}, bytes(b) {}

  explicit JPP_ALWAYS_INLINE InputCodepoint(StringPiece sp) noexcept
      : codepoint(getCodepoint(sp.ubegin(), sp.uend()).codepoint),
        charClass{getCodeType(codepoint)},
        bytes(sp) {}
};

Status preprocessRawData(StringPiece utf8data,
                         std::vector<InputCodepoint>* result);

i32 numCodepoints(StringPiece utf8);

}  // namespace chars
}  // namespace jumanpp

#endif  // JUMANPP_CHARACTERS_HPP

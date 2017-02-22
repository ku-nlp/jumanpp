//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "characters.hpp"
#include <util/flatset.h>

namespace jumanpp {
namespace chars {

const util::FlatSet<char32_t> lowercase{
    0x3041,  // ぁ
    0x3043,  // ぃ
    0x3045,  // ぅ
    0x3047,  // ぇ
    0x3049,  // ぉ
    0x3063,  // っ
    0x3083,  // ゃ
    0x3085,  // ゅ
    0x3087,  // ょ
    0x308E,  // ゎ
    0x3095,  // ゕ HIRAGANA LETTER SMALL KA
    0x3096,  // ゖ HIRAGANA LETTER SMALL KE
    0x30A1,  // ァ
    0x30A3,  // ィ
    0x30A5,  // ゥ
    0x30A7,  // ェ
    0x30A9,  // ォ
    0x30C3,  // ッ
    0x30E3,  // ャ
    0x30E5,  // ュ
    0x30E7,  // ョ
    0x30EE,  // ヮ
    0x30F5,  // ヵ
    0x30F6   // ヶ
};

const util::FlatSet<char32_t> brackets{
    0x0028, 0x0029,  // PARENTHESIS
    0x005B, 0x005D,  // SQUARE BRACKET
    0x007B, 0x007D,  // CURLY BRACKET
    0x0F3A, 0x0F3B,  // TIBETAN MARK GUG RTAGS GYON
    0x0F3C, 0x0F3D,  // TIBETAN MARK ANG KHANG GYON
    0x169B, 0x169C,  // OGHAM FEATHER MARK
    0x2045, 0x2046,  // SQUARE BRACKET WITH QUILL
    0x207D, 0x207E,  // SUPERSCRIPT PARENTHESIS
    0x208D, 0x208E,  // SUBSCRIPT PARENTHESIS
    0x2308, 0x2309,  // CEILING
    0x230A, 0x230B,  // FLOOR
    0x2329, 0x232A,  // ANGLE BRACKET
    0x2768, 0x2769,  // MEDIUM PARENTHESIS ORNAMENT
    0x276A, 0x276B,  // MEDIUM FLATTENED PARENTHESIS ORNAMENT
    0x276C, 0x276D,  // MEDIUM ANGLE BRACKET ORNAMENT
    0x276E, 0x276F,  // HEAVY ANGLE QUOTATION MARK ORNAMENT
    0x2770, 0x2771,  // HEAVY ANGLE BRACKET ORNAMENT
    0x2772, 0x2773,  // LIGHT TORTOISE SHELL BRACKET ORNAMENT
    0x2774, 0x2775,  // MEDIUM CURLY BRACKET ORNAMENT
    0x27C5, 0x27C6,  // S-SHAPED BAG DELIMITER
    0x27E6, 0x27E7,  // MATHEMATICAL WHITE SQUARE BRACKET
    0x27E8, 0x27E9,  // MATHEMATICAL ANGLE BRACKET
    0x27EA, 0x27EB,  // MATHEMATICAL DOUBLE ANGLE BRACKET
    0x27EC, 0x27ED,  // MATHEMATICAL WHITE TORTOISE SHELL BRACKET
    0x27EE, 0x27EF,  // MATHEMATICAL FLATTENED PARENTHESIS
    0x2983, 0x2984,  // WHITE CURLY BRACKET
    0x2985, 0x2986,  // WHITE PARENTHESIS
    0x2987, 0x2988,  // Z NOTATION  IMAGE BRACKET
    0x2989, 0x298A,  // Z NOTATION  BINDING BRACKET
    0x298B, 0x298C,  // SQUARE BRACKET WITH UNDERBAR
    0x298D, 0x2990,  // SQUARE BRACKET WITH TICK IN TOP CORNER
    0x298F, 0x298E,  // SQUARE BRACKET WITH TICK IN BOTTOM CORNER
    0x2991, 0x2992,  // ANGLE BRACKET WITH DOT
    0x2993, 0x2994,  // ARC LESS-THAN BRACKET
    0x2995, 0x2996,  // DOUBLE  ARC GREATER-THAN BRACKET
    0x2997, 0x2998,  // BLACK TORTOISE SHELL BRACKET
    0x29D8, 0x29D9,  // WIGGLY FENCE
    0x29DA, 0x29DB,  // DOUBLE WIGGLY FENCE
    0x29FC, 0x29FD,  // CURVED ANGLE BRACKET
    0x2E22, 0x2E23,  // TOP HALF BRACKET
    0x2E24, 0x2E25,  // BOTTOM  HALF BRACKET
    0x2E26, 0x2E27,  // SIDEWAYS U BRACKET
    0x2E28, 0x2E29,  // DOUBLE PARENTHESIS
    0x3008, 0x3009,  // ANGLE BRACKET
    0x300A, 0x300B,  // DOUBLE ANGLE BRACKET
    0x300C, 0x300D,  // CORNER BRACKET
    0x300E, 0x300F,  // WHITE CORNER BRACKET
    0x3010, 0x3011,  // BLACK LENTICULAR BRACKET
    0x3014, 0x3015,  // TORTOISE SHELL BRACKET
    0x3016, 0x3017,  // WHITE LENTICULAR BRACKET
    0x3018, 0x3019,  // WHITE TORTOISE SHELL BRACKET
    0x301A, 0x301B,  // WHITE SQUARE BRACKET
    0xFE59, 0xFE5A,  // SMALL PARENTHESIS
    0xFE5B, 0xFE5C,  // SMALL CURLY BRACKET
    0xFE5D, 0xFE5E,  // SMALL TORTOISE SHELL BRACKET
    0xFF08, 0xFF09,  // FULLWIDTH PARENTHESIS
    0xFF3B, 0xFF3D,  // FULLWIDTH SQUARE BRACKET
    0xFF5B, 0xFF5D,  // FULLWIDTH CURLY BRACKET
    0xFF5F, 0xFF60,  // FULLWIDTH WHITE PARENTHESIS
    0xFF62, 0xFF63,  // HALFWIDTH CORNER BRACKET
};

bool toCodepoint(StringPiece::iterator &input_itr,
                 StringPiece::iterator itr_end2, char32_t *result) noexcept {
  char32_t u;
  const u8 *itr = reinterpret_cast<const u8 *>(input_itr);
  const u8 *itr_end = reinterpret_cast<const u8 *>(itr_end2);

  if (*itr > 0x00ef) { /* 4 bytes */
    JPP_RET_CHECK(itr_end - itr >= 4);
    JPP_RET_CHECK(((*itr & ~0x7) ^ 0xf0) == 0);
    u = (*(itr++) & 0x07) << 18;
    JPP_RET_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3f) << 12;
    JPP_RET_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3f) << 6;
    JPP_RET_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3f);
  } else if (*itr > 0xdf) { /* 3 bytes */
    JPP_RET_CHECK(itr_end - itr >= 3);
    JPP_RET_CHECK(((*itr & ~0xf) ^ 0xe0) == 0);
    u = (*(itr++) & 0x0f) << 12;
    JPP_RET_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= (*(itr++) & 0x3f) << 6;
    JPP_RET_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= *(itr++) & 0x3f;
  } else if (*itr > 0x7f) { /* 2 bytes */
    JPP_RET_CHECK(itr_end - itr >= 2);
    JPP_RET_CHECK(((*itr & ~0x1f) ^ 0xc0) == 0);
    u = (*(itr++) & 0x1f) << 6;
    JPP_RET_CHECK(((*itr & ~0x3f) ^ 0x80) == 0);
    u |= *(itr++) & 0x3f;
  } else { /* 1 byte */
    JPP_RET_CHECK(itr_end - itr >= 1);
    JPP_RET_CHECK((*itr & ~0x7f) == 0);
    u = *(itr++) & 0x7f;
  }
  *result = u;
  input_itr = reinterpret_cast<StringPiece::pointer_t>(itr);
  return true;
}

using Codepoint = char32_t;

template <Codepoint... haystack>
bool inSet(Codepoint needle) {
  static const util::FlatSet<Codepoint> staticHaystack{haystack...};
  return staticHaystack.count(needle) != 0;
}

CharacterClass getCodeType(char32_t input) noexcept {
  Codepoint code = static_cast<Codepoint>(input);
  /* SPACE */
  if (inSet<0x20,                                         // space
            0x3000,                                       // fullwidth space
            0xA0, 0x1680, 0x180E, 0x202F, 0x205F, 0xFEFF  // other spaces
            >(code) ||
      (0x2000 <= code && code <= 0x200B)) { /*Other spaces on unicode*/
    return CharacterClass::SPACE;
  }
  /* IDEOGRAPHIC PUNCTUATIONS (、。) */
  else if (code > 0x3000 && code < 0x3003) {
    return CharacterClass::IDEOGRAPHIC_PUNC;
  } else if (0x337B <= code && code <= 0x337E) { /* ㍻㍼㍽㍾ */
    return CharacterClass::SYMBOL | CharacterClass::ERA;
  }
  /* HIRAGANA */
  else if ((code > 0x303f && code < 0x30a0) ||
           /* Iteration marks ゝゞ*/
           code == 0x309D || code == 0x309E || code == 0x30FF /* YORI ゟ */) {
    if (lowercase.count(code) == 1)
      return CharacterClass::HIRAGANA | CharacterClass::LOWER_CASE;
    else
      return CharacterClass::HIRAGANA;
  }
  /* KATAKANA and KATAKANA symbols */
  else if ((code > 0x309f && code < 0x30fb) ||
           /* Iteration marks ヽヾ */
           code == 0x30FD || code == 0x30FE || code == 0x30FF /* KOTO ヿ*/) {
    if (lowercase.count(code) == 1)
      return CharacterClass::KATAKANA | CharacterClass::LOWER_CASE;
    else
      return CharacterClass::KATAKANA;
  } else if (code ==
             0x30fc) {  // KATAKANA-HIRAGANA PROLONGED SOUND MARK (0x30fc)
    return CharacterClass::KATAKANA | CharacterClass::CHOON;
  } else if (code == 0xFF70) {  // Half-widths HIRAGANA-KATAKANA PROLONGED SOUND
                                // MARK (U+FF70
    return CharacterClass::HANKAKU_KANA | CharacterClass::CHOON;
  } else if (0xFF66 <= code &&
             code <= 0xFF9F) {  // Half-widths KATAKANA (0xFF66-0xFF9F)
    return CharacterClass::HANKAKU_KANA;
  } else if (code == 0x30fc) {  // "〜"(0x301C)  ⁓ (U+2053)、Full-width tilde:
                                // ～ (U+FF5E)、tilde operator: ∼ (U+223C)
    return CharacterClass::CHOON;
  }
  /* "・"(0x30fb) "· 0x 00B7 */
  else if (code == 0x00B7 || code == 0x30fb) {
    return CharacterClass::MIDDLE_DOT;
  }
  /* "，"(0xff0c), "," (0x002C) */
  else if (code == 0x002C || code == 0xff0c) {
    return CharacterClass::COMMA;
  }
  /* "/"0x002F, "／"(0xff0f) */
  else if (code == 0x002F || code == 0xff0f) {
    return CharacterClass::SLASH;
  }
  /* ":" 0x003A "："(0xff1a) */
  else if (code == 0x003A || code == 0xff1a) {
    return CharacterClass::COLON;
  }
  /* PRIOD */
  else if (code == 0xff0e) {
    return CharacterClass::PERIOD;
  }
  /* FIGURE (0-9, ０-９) */  // Special numbers such as ① and ⅷ are treated as
                             // symbols.
  else if ((code > 0x2f && code < 0x3a) || (code > 0xff0f && code < 0xff1a)) {
    return CharacterClass::FIGURE;
  }
  /* KANJI_FIGURE (○一七万三九二五亿六八十千四百零, ％, 点余多) */
  else if (code == 0x25cb ||  //○ Full-widths circle symbol (sometimes it is
                              // used instead of zero)
           code == 0x3007 ||  //〇
           code == 0x96f6 ||  //零
           code == 0x4e00 ||  //一
           code == 0x4e8c ||  //二
           code == 0x4e09 ||  //三
           code == 0x56db ||  //四
           code == 0x4e94 ||  //五
           code == 0x516d ||  //六
           code == 0x4e03 ||  //七
           code == 0x516b ||  //八
           code == 0x4e5d ||  //九
           false) {
    return CharacterClass::KANJI_FIGURE;
  } else if (code == 0x5341 ||  //十
             code == 0x767e ||  //百
             code == 0x5343 ||  //千
             code == 0x4e07 ||  //万
             code == 0x5104 ||  //億
             code == 0x5146 ||  //兆
             code == 0x6570 ||  //数
             code == 0x4F55 ||  //何
             code == 0x5E7E ||  //幾
             false) {
    if (code == 0x6570 || code == 0x4F55 || code == 0x5E7E) /* 数，何，幾 */
      return CharacterClass::KANJI_FIGURE | CharacterClass::FIGURE_EXCEPTION;
    else
      /* DIGITs */
      return CharacterClass::KANJI_FIGURE | CharacterClass::FIGURE_DIGIT;
  }
  /* ALPHABET (A-Z, a-z, Umlaut etc., Ａ-Ｚ, ａ-ｚ) */
  else if ((code > 0x40 && code < 0x5b) || (code > 0x60 && code < 0x7b) ||
           (code > 0xbf && code < 0x0100) || (code > 0xff20 && code < 0xff3b) ||
           (code > 0xff40 && code < 0xff5b)) {
    return CharacterClass::ALPH;
  }
  /* CJK Unified Ideographs and "々" and "〇"*/
  else if ((code > 0x4dff && code < 0xa000) || code == 0x3005 ||
           code == 0x3007) {
    return CharacterClass::KANJI;
  } else if (brackets.count(code) == 1) {
    /* Brackets */
    return CharacterClass::BRACKET;
  } else {
    return CharacterClass::SYMBOL;
  }
}

Status preprocessRawData(StringPiece utf8data,
                         std::vector<InputCodepoint> *result) {
  auto itr = utf8data.begin();
  while (itr < utf8data.end()) {
    char32_t unicode;
    auto begin = itr;
    bool ret = toCodepoint(itr, utf8data.end(), &unicode);
    if (!ret) return Status::InvalidState() << "Invalid UTF8 sequence";

    CharacterClass cc = getCodeType(unicode);
    result->emplace_back(InputCodepoint{unicode, cc, StringPiece(begin, itr)});
  }
  return Status::Ok();
}

}  // chars
}  // jumanpp

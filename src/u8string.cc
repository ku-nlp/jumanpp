
#include "u8string.h"

std::ostream &operator<<(std::ostream &os, Morph::U8string &u) {
    os << u.byte_str;
    return os;
};

namespace Morph {
const std::unordered_set<std::string> U8string::lowercase{
    //{{{
    "ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゎ", "ヵ", "ァ", "ィ", "ゥ", "ェ", "ォ",
    "ヮ", "っ", "ッ", "ん", "ン", "ゃ", "ャ", "ゅ", "ュ", "ょ", "ョ"}; //}}}
const std::unordered_set<unsigned int> U8string::brackets{
    //{{{
    0x0028, 0x0029, // PARENTHESIS
    0x005B, 0x005D, // SQUARE BRACKET
    0x007B, 0x007D, // CURLY BRACKET
    0x0F3A, 0x0F3B, // TIBETAN MARK GUG RTAGS GYON
    0x0F3C, 0x0F3D, // TIBETAN MARK ANG KHANG GYON
    0x169B, 0x169C, // OGHAM FEATHER MARK
    0x2045, 0x2046, // SQUARE BRACKET WITH QUILL
    0x207D, 0x207E, // SUPERSCRIPT PARENTHESIS
    0x208D, 0x208E, // SUBSCRIPT PARENTHESIS
    0x2308, 0x2309, // CEILING
    0x230A, 0x230B, // FLOOR
    0x2329, 0x232A, // ANGLE BRACKET
    0x2768, 0x2769, // MEDIUM PARENTHESIS ORNAMENT
    0x276A, 0x276B, // MEDIUM FLATTENED PARENTHESIS ORNAMENT
    0x276C, 0x276D, // MEDIUM ANGLE BRACKET ORNAMENT
    0x276E, 0x276F, // HEAVY ANGLE QUOTATION MARK ORNAMENT
    0x2770, 0x2771, // HEAVY ANGLE BRACKET ORNAMENT
    0x2772, 0x2773, // LIGHT TORTOISE SHELL BRACKET ORNAMENT
    0x2774, 0x2775, // MEDIUM CURLY BRACKET ORNAMENT
    0x27C5, 0x27C6, // S-SHAPED BAG DELIMITER
    0x27E6, 0x27E7, // MATHEMATICAL WHITE SQUARE BRACKET
    0x27E8, 0x27E9, // MATHEMATICAL ANGLE BRACKET
    0x27EA, 0x27EB, // MATHEMATICAL DOUBLE ANGLE BRACKET
    0x27EC, 0x27ED, // MATHEMATICAL WHITE TORTOISE SHELL BRACKET
    0x27EE, 0x27EF, // MATHEMATICAL FLATTENED PARENTHESIS
    0x2983, 0x2984, // WHITE CURLY BRACKET
    0x2985, 0x2986, // WHITE PARENTHESIS
    0x2987, 0x2988, // Z NOTATION  IMAGE BRACKET
    0x2989, 0x298A, // Z NOTATION  BINDING BRACKET
    0x298B, 0x298C, // SQUARE BRACKET WITH UNDERBAR
    0x298D, 0x2990, // SQUARE BRACKET WITH TICK IN TOP CORNER
    0x298F, 0x298E, // SQUARE BRACKET WITH TICK IN BOTTOM CORNER
    0x2991, 0x2992, // ANGLE BRACKET WITH DOT
    0x2993, 0x2994, // ARC LESS-THAN BRACKET
    0x2995, 0x2996, // DOUBLE  ARC GREATER-THAN BRACKET
    0x2997, 0x2998, // BLACK TORTOISE SHELL BRACKET
    0x29D8, 0x29D9, // WIGGLY FENCE
    0x29DA, 0x29DB, // DOUBLE WIGGLY FENCE
    0x29FC, 0x29FD, // CURVED ANGLE BRACKET
    0x2E22, 0x2E23, // TOP HALF BRACKET
    0x2E24, 0x2E25, // BOTTOM  HALF BRACKET
    0x2E26, 0x2E27, // SIDEWAYS U BRACKET
    0x2E28, 0x2E29, // DOUBLE PARENTHESIS
    0x3008, 0x3009, // ANGLE BRACKET
    0x300A, 0x300B, // DOUBLE ANGLE BRACKET
    0x300C, 0x300D, // CORNER BRACKET
    0x300E, 0x300F, // WHITE CORNER BRACKET
    0x3010, 0x3011, // BLACK LENTICULAR BRACKET
    0x3014, 0x3015, // TORTOISE SHELL BRACKET
    0x3016, 0x3017, // WHITE LENTICULAR BRACKET
    0x3018, 0x3019, // WHITE TORTOISE SHELL BRACKET
    0x301A, 0x301B, // WHITE SQUARE BRACKET
    0xFE59, 0xFE5A, // SMALL PARENTHESIS
    0xFE5B, 0xFE5C, // SMALL CURLY BRACKET
    0xFE5D, 0xFE5E, // SMALL TORTOISE SHELL BRACKET
    0xFF08, 0xFF09, // FULLWIDTH PARENTHESIS
    0xFF3B, 0xFF3D, // FULLWIDTH SQUARE BRACKET
    0xFF5B, 0xFF5D, // FULLWIDTH CURLY BRACKET
    0xFF5F, 0xFF60, // FULLWIDTH WHITE PARENTHESIS
    0xFF62, 0xFF63, // HALFWIDTH CORNER BRACKET
};                  //}}}

int U8string::to_unicode(std::vector<unsigned char> &c) { //{{{
    int unicode = 0;

    if (c[0] > 0x00ef) {      /* 4 bytes */
        return 0;             // 無視
    } else if (c[0] > 0xdf) { /* 3 bytes */
        unicode = (c[0] & 0x0f) << 12;
        unicode += (c[1] & 0x3f) << 6;
        unicode += c[2] & 0x3f;
        return unicode;
    } else if (c[0] > 0x7f) { /* 2 bytes */
        unicode = (c[0] & 0x1f) << 6;
        unicode += c[1] & 0x3f;
        return unicode;
    } else { /* 1 byte */
        return c[0];
    }
}; //}}}

unsigned long U8string::check_unicode_char_type(int code) { //{{{
    /* SPACE */
    if (code == 0x20 /* space*/ || code == 0x3000 /*全角スペース*/ ||
        code == 0x00A0 || code == 0x1680 ||
        code == 0x180E || /*その他のunicode上のスペース*/
        (0x2000 <= code && code <= 0x200B) || code == 0x202F ||
        code == 0x205F || code == 0xFEFF) {
        return SPACE;
    }
    /* IDEOGRAPHIC PUNCTUATIONS (、。) */
    else if (code > 0x3000 && code < 0x3003) {
        return IDEOGRAPHIC_PUNC;
    } else if (0x337B <= code && code <= 0x337E) { /* ㍻㍼㍽㍾ */
        return SYMBOL + ERA;
    }
    /* HIRAGANA */
    else if (code > 0x303f && code < 0x30a0) {
        return HIRAGANA;
    }
    /* KATAKANA and  */
    else if (code > 0x309f && code < 0x30fb) {
        return KATAKANA;
    } else if (code == 0x30fc) { // "ー"(0x30fc)
        return KATAKANA + CHOON;
    } else if (0x00A1 <= code && code <= 0x00DF) { //半角カナ(0xA1-0xDF)
        return HANKAKU_KANA;
    } else if (code == 0xFF70) { //半角長音(U+FF70
        return HANKAKU_KANA + CHOON;
    } else if (code == 0x30fc) { // "〜"(0x301C)  ⁓ (U+2053)、fullwidth tilde:
                                 // ～ (U+FF5E)、tilde operator: ∼ (U+223C)
        return CHOON;
    }
    /* "・"(0x30fb) "· 0x 00B7 */
    else if (code == 0x00B7 || code == 0x30fb) {
        return MIDDLE_DOT;
    }
    /* "，"(0xff0c), "," (0x002C) */
    else if (code == 0x002C || code == 0xff0c) {
        return COMMA;
    }
    /* "/"0x002F, "／"(0xff0f) */
    else if (code == 0x002F || code == 0xff0f) {
        return SLASH;
    }
    /* ":" 0x003A "："(0xff1a) */
    else if (code == 0x003A || code == 0xff1a) {
        return COLON;
    }
    /* PRIOD */
    else if (code == 0xff0e) {
        return PERIOD;
    }
    /* FIGURE (0-9, ０-９) */ //①，ⅷ などはどうするか
    else if ((code > 0x2f && code < 0x3a) || (code > 0xff0f && code < 0xff1a)) {
        return FIGURE;
    }
    /* KANJI_FIGURE (○一七万三九二五亿六八十千四百零, ％, 点余多) */
    else if (code == 0x25cb || //○(全角丸)
             code == 0x3007 || //〇
             code == 0x96f6 || //零
             code == 0x4e00 || //一
             code == 0x4e8c || //二
             code == 0x4e09 || //三
             code == 0x56db || //四
             code == 0x4e94 || //五
             code == 0x516d || //六
             code == 0x4e03 || //七
             code == 0x516b || //八
             code == 0x4e5d || //九
             false) {
        return KANJI_FIGURE;
    } else if (code == 0x5341 || //十
               code == 0x767e || //百
               code == 0x5343 || //千
               code == 0x4e07 || //万
               code == 0x5104 || //億
               code == 0x5146 || //兆
               code == 0x6570 || //数
               code == 0x4F55 || //何
               code == 0x5E7E || //幾
               false) {
        if (code == 0x6570 || code == 0x4F55 || code == 0x5E7E) // 数，何，幾
            return KANJI_FIGURE + FIGURE_EXCEPTION;
        else
            return KANJI_FIGURE + FIGURE_DIGIT;
    }
    /* ALPHABET (A-Z, a-z, Umlaut etc., Ａ-Ｚ, ａ-ｚ) */
    else if ((code > 0x40 && code < 0x5b) || (code > 0x60 && code < 0x7b) ||
             (code > 0xbf && code < 0x0100) ||
             (code > 0xff20 && code < 0xff3b) ||
             (code > 0xff40 && code < 0xff5b)) {
        return ALPH;
    }
    /* CJK Unified Ideographs and "々" and "〇"*/
    else if ((code > 0x4dff && code < 0xa000) || code == 0x3005 ||
             code == 0x3007) {
        return KANJI;
    } else if (brackets.count(code) == 1) { // 括弧，引用符
        return BRACKET;
    } else {
        return SYMBOL;
    }
}; //}}}
}

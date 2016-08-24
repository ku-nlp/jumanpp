#include "common.h"

// TODO: 廃止

namespace Morph {

// return the bytes of a char
int utf8_bytes(unsigned char *ucp) {
    unsigned char c = *ucp;

    if (c > 0xfb) { /* 6 bytes */
        return 6;
    } else if (c > 0xf7) { /* 5 bytes */
        return 5;
    } else if (c > 0xef) { /* 4 bytes */
        return 4;
    } else if (c > 0xdf) { /* 3 bytes */
        return 3;
    } else if (c > 0x7f) { /* 2 bytes */
        return 2;
    } else { /* 1 byte */
        return 1;
    }
}

// utf8 文字列の長さを計る
size_t utf8_length(const char *ucp) {
    size_t char_length = 0;
    size_t byte_length = strlen(ucp);
    for (unsigned int pos = 0; pos < byte_length;
         pos += utf8_bytes((unsigned char *)(ucp + pos))) {
        char_length += 1;
    }
    return char_length;
}

// return the bytes of a char
unsigned short utf8_chars(unsigned char *ucp, size_t byte_len) {
    unsigned short char_num = 0;
    for (size_t pos = 0; pos < byte_len; pos += utf8_bytes(ucp + pos)) {
        char_num++;
    }
    return char_num;
}

// return the pointer of the specified char (if 1 is specified, return the
// second char pointer)
unsigned char *get_specified_char_pointer(unsigned char *ucp, size_t byte_len,
                                          unsigned short specified_char_num) {
    unsigned short char_num = 0;
    size_t last_pos = 0;
    for (size_t pos = 0; pos < byte_len; pos += utf8_bytes(ucp + pos)) {
        if (char_num == specified_char_num) {
            return ucp + pos;
        }
        char_num++;
        last_pos = pos;
    }
    return ucp + last_pos;
}

unsigned int check_unicode_char_type(int code) {
    /* SPACE */
    if (code == 0x20 || code == 0x3000) {
        return TYPE_SPACE;
    }
    /* IDEOGRAPHIC PUNCTUATIONS (、。) */
    else if (code > 0x3000 && code < 0x3003) {
        return TYPE_IDEOGRAPHIC_PUNC;
    }
    /* HIRAGANA */
    else if (code > 0x303f && code < 0x30a0) {
        return TYPE_HIRAGANA;
    }
    /* KATAKANA and "ー"(0x30fc) */
    else if ((code > 0x309f && code < 0x30fb) || code == 0x30fc) {
        return TYPE_KATAKANA;
    }
    /* "・"(0x30fb) */
    else if (code == 0x30fb) {
        return TYPE_MIDDLE_DOT;
    }
    /* "，"(0xff0c) */
    else if (code == 0xff0c) {
        return TYPE_COMMA;
    }
    /* "／"(0xff0f) */
    else if (code == 0xff0f) {
        return TYPE_SLASH;
    }
    /* "："(0xff1a) */
    else if (code == 0xff1a) {
        return TYPE_COLON;
    }
    /* PRIOD */
    else if (code == 0xff0e) {
        return TYPE_PERIOD;
    }
    /* FIGURE (0-9, ０-９) */
    else if ((code > 0x2f && code < 0x3a) || (code > 0xff0f && code < 0xff1a)) {
        return TYPE_FIGURE;
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
             code == 0x5341 || //十
             code == 0x767e || //百
             code == 0x5343 || //千
             code == 0x4e07 || //万
             code == 0x5104 || //億
             code == 0x5146 || //兆
             code == 0xff0c || //，
             // code == 0xff05 ||//％
             // 年月日: code == 0x5e74 || code == 0x6708 || code == 0x65e5 ||
             // code == 0x4ebf || //??
             // code == 0x4f59 || //余
             // code == 0x591a || //多
             // code == 0x70b9 //点
             false) {
        return TYPE_KANJI_FIGURE;
    } else if (code == 0x6570 || //数
               code == 0x4F55 || //何
               code == 0x5E7E || //幾
               false) {
        return (TYPE_KANJI_FIGURE + TYPE_FIGURE_EXCEPTION);
        /* ALPHABET (A-Z, a-z, Umlaut etc., Ａ-Ｚ, ａ-ｚ) */
    } else if ((code > 0x40 && code < 0x5b) || (code > 0x60 && code < 0x7b) ||
               (code > 0xbf && code < 0x0100) ||
               (code > 0xff20 && code < 0xff3b) ||
               (code > 0xff40 && code < 0xff5b)) {
        return TYPE_ALPH;
    }
    /* CJK Unified Ideographs and "々" and "〇"*/
    else if ((code > 0x4dff && code < 0xa000) || code == 0x3005 ||
             code == 0x3007) {
        return TYPE_KANJI;
    } else {
        return TYPE_SYMBOL;
    }
}

// int check_code(U_CHAR *cp, int pos) {
//    int	code;
//
//    if ( cp[pos] == '\0')			return 0;
//    else if ( cp[pos] == KUUHAKU )		return KUUHAKU;
//
//#ifdef IO_ENCODING_EUC
//    else if ( cp[pos] < HANKAKU )		return HANKAKU;
//
//    code = cp[pos]*256 + cp[pos+1];
//
//    if ( code == PRIOD ) 			return PRIOD;
//    else if ( code == CHOON ) 			return CHOON;
//    else if ( code < KIGOU ) 			return KIGOU;
//    else if ( code < SUJI ) 			return SUJI;
//    else if ( code < ALPH )			return ALPH;
//    else if ( code < HIRAGANA ) 		return HIRAGANA;
//    else if ( code < KATAKANA ) 		return KATAKANA;
//    else if ( code < GR ) 			return GR;
//    else return KANJI;
//#else
//#ifdef IO_ENCODING_SJIS
//    else if ( cp[pos] < HANKAKU )		return HANKAKU;
//
//    code = cp[pos]*256 + cp[pos+1];
//
//    if ( code == 0x8144 ) 			return PRIOD;
//    else if ( code == 0x815b ) 			return CHOON;
//    else if ( code < 0x8200 ) 			return KIGOU;
//    else if ( code < 0x8260 ) 			return SUJI;
//    else if ( code < 0x829f )			return ALPH;
//    else if ( code < 0x8300 ) 			return HIRAGANA;
//    else if ( code < 0x839f ) 			return KATAKANA;
//    else if ( code < 0x8800 ) 			return GR;
//    else return KANJI;
//#else /* UTF-8 */
//    return check_utf8_char_type(cp + pos);
//#endif
//#endif
//}

// check the code of a char
unsigned int check_utf8_char_type(const char *ucp) {
    // char cp[3];
    // strncpy(cp, ucp,3);
    return check_utf8_char_type((unsigned char *)ucp);
}

unsigned int check_utf8_char_type(unsigned char *ucp) {
    int code = 0;
    int unicode;
    unsigned char c = *ucp;

    if (c > 0xfb) { /* 6 bytes */
        code = 0;
    } else if (c > 0xf7) { /* 5 bytes */
        code = 0;
    } else if (c > 0xef) { /* 4 bytes */
        code = 0;
    } else if (c > 0xdf) { /* 3 bytes */
        unicode = (c & 0x0f) << 12;
        c = *(ucp + 1);
        unicode += (c & 0x3f) << 6;
        c = *(ucp + 2);
        unicode += c & 0x3f;
        code = check_unicode_char_type(unicode);
    } else if (c > 0x7f) { /* 2 bytes */
        unicode = (c & 0x1f) << 6;
        c = *(ucp + 1);
        unicode += c & 0x3f;
        code = check_unicode_char_type(unicode);
    } else { /* 1 byte */
        code = check_unicode_char_type(c);
    }

    return code;
}

unsigned int check_char_family(unsigned char *ucp) {
    return check_char_family(check_utf8_char_type(ucp));
}

unsigned int check_char_family(unsigned int char_type) {
    if (char_type & TYPE_FAMILY_KANJI) {
        return TYPE_FAMILY_KANJI;
    } else if (char_type & TYPE_FAMILY_SPACE) {
        return TYPE_FAMILY_SPACE;
    } else if (char_type & TYPE_FAMILY_ALPH) {
        return TYPE_FAMILY_ALPH;
    } else if (char_type & TYPE_FAMILY_PURE_FIGURE) { // ピリオドなどは含まない
        return TYPE_FAMILY_FIGURE;
        //} else if (char_type & TYPE_FIGURE) { // figure only
        //    return TYPE_FIGURE;
    } else { // char_type & TYPE_FAMILY_SYMBOL
        return TYPE_FAMILY_SYMBOL;
    }
}

bool compare_char_type_in_family(unsigned int char_type,
                                 unsigned int family_pattern) {
    if (char_type & family_pattern) {
        return true;
    } else {
        return false;
    }
}

// exceptional expression
// キロとかここに入れればいい？ペタはjumanでは入ってなかった
// 数億，数円，数時間をどうするか
unsigned int check_exceptional_chars_in_figure(const char *cp,
                                               unsigned int rest_byte_len) {
    if (rest_byte_len >= EXCEPTIONAL_FIGURE_EXPRESSION_LENGTH && //分の
        !strncmp(cp, EXCEPTIONAL_FIGURE_EXPRESSION,
                 EXCEPTIONAL_FIGURE_EXPRESSION_LENGTH)) {
        return 2;
    } else if (rest_byte_len >=
                   EXCEPTIONAL_FIGURE_EXPRESSION_LENGTH_2 && //ぶんの
               strncmp(cp, EXCEPTIONAL_FIGURE_EXPRESSION_2,
                       EXCEPTIONAL_FIGURE_EXPRESSION_LENGTH_2) == 0) {
        return 3;
    } else if (rest_byte_len >= 6 && !strncmp(cp, "キロ", 6)) {
        return 2;
    } else if (rest_byte_len >= 6 && !strncmp(cp, "メガ", 6)) {
        return 2;
    } else if (rest_byte_len >= 6 && !strncmp(cp, "ギガ", 6)) {
        return 2;
    } else if (rest_byte_len >= 6 && !strncmp(cp, "テラ", 6)) {
        return 2;
    } else if (rest_byte_len >= 6 && !strncmp(cp, "ミリ", 6)) {
        return 2;
    } else {
        return 0;
    }
}

// すべて数字かどうかチェック *u8strにもうつす
size_t is_suuji(unsigned char *ucp) {
    size_t byte_length = strlen((const char *)ucp);
    if (byte_length == 0)
        return false;
    for (unsigned int pos = 0; pos < byte_length;
         pos += utf8_bytes((unsigned char *)(ucp + pos))) {
        if ((check_char_family(
                 check_utf8_char_type((unsigned char *)(ucp + pos))) &
             TYPE_FAMILY_FIGURE) == TYPE_FAMILY_FIGURE)
            return false;
    }
    return true;
}
}


#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iterator>
#include <string>
#include <unordered_set>


#define TYPE_FAMILY_FIGURE	14392 // TYPE_FIGURE + TYPE_PERIOD + TYPE_MIDDLE_DOT + TYPE_KANJI_FIGURE + TYPE_SLASH + TYPE_COLON
#define TYPE_FAMILY_PUNC	82 // TYPE_PERIOD + TYPE_COMMA + TYPE_IDEOGRAPHIC_PUNC
#define TYPE_FAMILY_ALPH_PUNC	12432 // TYPE_ALPH + TYPE_PERIOD + TYPE_SLASH + TYPE_COLON
#define TYPE_FAMILY_PUNC_SYMBOL	12658 // TYPE_PERIOD + TYPE_COMMA + TYPE_IDEOGRAPHIC_PUNC + TYPE_MIDDLE_DOT + TYPE_SYMBOL + TYPE_SLASH + TYPE_COLON

#define TYPE_FAMILY_SPACE	1   // TYPE_SPACE
#define TYPE_FAMILY_SYMBOL	256 // TYPE_SYMBOL
#define TYPE_FAMILY_ALPH	128 // TYPE_ALPH
#define TYPE_FAMILY_KANJI	2052 // TYPE_KANJI + TYPE_KANJI_FIGURE
#define TYPE_FAMILY_OTHERS	0

/******
 * U8string
 * utf8 の char バイト列を扱うクラス
 * コンストラクタでは utf8 の解釈は行わず，単に内部に std::string としてコピーするのみ．
 * 文字単位の扱いが必要になった時点で，バイト列を解釈し，文字ごとの配列を生成して内部的に用いる．
 * 単に string として扱ってもそれほどオーバーヘッドがない（はず）．
 * string でよく使いそうな昨日のみをラップしてある．(find, substr, c_str)
 * size(), length() は何のサイズを返すべきか曖昧なので, 実装しない．代わりに byte_size, char_size を用意した．
 * morita(2015/01/16)
 ******/

//エラー処理が不十分
//空文字のみを渡された場合，"", NULL, 等を渡された場合，

namespace Morph{ class U8string; }// 次の関数をGlobalに定義する際に必要．
std::ostream& operator << (std::ostream& os, Morph::U8string& u);

namespace Morph{

class U8string{//{{{
    friend std::ostream& (::operator <<) (std::ostream&, Morph::U8string&);//cout 用
    public: 
    // 定数群, or で繋げたいのでenum にはしない．
    static constexpr unsigned long SPACE           =0x00000001;
    static constexpr unsigned long IDEOGRAPHIC_PUNC=0x00000002; // 、。
    static constexpr unsigned long KANJI           =0x00000004;
    static constexpr unsigned long FIGURE          =0x00000008;
    static constexpr unsigned long PERIOD          =0x00000010; // ．
    static constexpr unsigned long MIDDLE_DOT      =0x00000020; // ・
    static constexpr unsigned long COMMA           =0x00000040; //　，
    static constexpr unsigned long ALPH            =0x00000080;
    static constexpr unsigned long SYMBOL          =0x00000100;
    static constexpr unsigned long KATAKANA        =0x00000200;
    static constexpr unsigned long HIRAGANA        =0x00000400;
    static constexpr unsigned long KANJI_FIGURE    =0x00000800; // KANJI+FIGURE にするべき？
    static constexpr unsigned long SLASH           =0x00001000;
    static constexpr unsigned long COLON           =0x00002000;
    static constexpr unsigned long ERA             =0x00004000; // ㍻ 
    static constexpr unsigned long CHOON           =0x00008000; // ー, 〜
    static constexpr unsigned long HANKAKU_KANA    =0x00010000; // 半角カタカナ

    static const std::unordered_set<std::string> lowercase;

    private: 
        std::string byte_str;
        std::vector<std::vector<unsigned char>> char_array;
        std::vector<size_t> char_index2byte_index;
        bool parsed = false;
    public:

        U8string(const std::string& c_str):parsed(false){//{{{
            byte_str = std::string(c_str);
            char_array.clear();
            char_index2byte_index.clear();
        }//}}}

        U8string(const U8string& u8str){//{{{
            parsed = u8str.parsed;
            byte_str = u8str.byte_str;
            char_array = u8str.char_array;
            char_index2byte_index = u8str.char_index2byte_index;
        }//}}}
          
        const std::string& str(){//{{{
            return byte_str;
        }//}}}

        size_t byte_size(){//{{{
            return byte_str.size();
        };//}}}

        size_t find( const std::string key_str, size_t index){//{{{
            return byte_str.find(key_str, index);
        };//}}}

        std::string byte_substr( size_t pos, size_t byte_length){//{{{
            return (byte_str.substr(pos,byte_length));
        };//}}}

        U8string operator+(const U8string& u8str){//{{{
            U8string new_str( byte_str + u8str.byte_str );
            return new_str;
        };//}}}
        
        U8string& operator+=(const U8string& u8str){//{{{
            parsed = false;
            byte_str += (u8str.byte_str);
            char_array.clear();
            char_index2byte_index.clear();
            return *this;
        };//}}}
        
        U8string& operator+=(const std::string& in_str){//{{{
            parsed = false;
            byte_str += (in_str);
            char_array.clear();
            char_index2byte_index.clear();
            return *this;
        };//}}}
         
        const char* c_str() { return byte_str.c_str();};

        inline void parse(){//{{{
            if(parsed)return;
            parsed=true;
            char_array.clear();
            char_index2byte_index.clear();
            for(size_t i=0,byte=utf8_bytes(byte_str[0]);i<byte_size();byte=utf8_bytes(byte_str[i]),i+=byte){
                char_array.emplace_back();
                //std::cerr << i << ":" << byte;
                for(char c:byte_str.substr(i,byte)){
                    //std::cerr << c << " ";
                    char_array.back().push_back(static_cast<unsigned char>(c));
                }
                //std::cerr << std::endl;
                char_index2byte_index.push_back(i);
            }
        };//}}}

        // 以下は要パース
        std::string operator []( size_t char_pos){//{{{
            parse();
            return (byte_str.substr(in_byte_index(char_pos), byte_size_at(char_pos)));
        };//}}}

        std::vector<unsigned char>& byte_array_at(unsigned int char_index){//{{{
            parse();
            return char_array[char_index];
        };//}}}
            
        std::string char_substr( size_t char_pos, size_t char_length){//{{{
            parse();
            return (byte_str.substr(in_byte_index(char_pos), in_byte_index(char_pos + char_length) - in_byte_index(char_pos)));
        };//}}}

        size_t char_size(){//{{{
            parse();
            return char_array.size();
        };//}}}

        // i 文字目が文字列の何バイト目かを返す
        size_t in_byte_index(size_t char_index){//{{{
            parse();
            return char_index2byte_index[char_index];
        }//}}}

        // i 文字目が何バイトの文字かを表す
        size_t byte_size_at(size_t i){//{{{
            return char_array[i].size();
        };//}}}

        unsigned int char_type_at(size_t char_ind){//{{{
            parse();
//            std::cerr << "key= " << byte_str << std::endl;
//            std::cerr << "char_array[" << char_ind <<" ] = " ;
//            for(auto a:char_array[char_ind]){
//                std::cerr << (unsigned long)a << " ";
//            }
//            std::cerr << " " << std::hex << to_unicode(char_array[char_ind]) << std::dec << std::endl;

            if(char_ind >= char_array.size()) return 0x0000;
            return check_code(char_array[char_ind]);
        }//}}}
        
        bool is_lower(size_t char_ind){//{{{
            parse();
            return lowercase.find(char_substr(char_ind, 1)) != lowercase.end();
        }//}}}

        bool is_choon(size_t char_ind){//{{{
            parse();
            return check_unicode_char_type( check_code(char_array[char_ind])) & CHOON;
        }//}}}
        // is_katakana, is_lower 等を追加予定
        //
    private:
        int next(int pos){//{{{
            if(!(0 < pos & pos < byte_str.size()))
                return -1;
            int bytes = utf8_bytes(byte_str[pos]);
            if(pos + bytes > byte_str.size())
                return -1;
            return pos + bytes;
        };//}}}

        int prev(int pos){//{{{
            if(!(0 < pos & pos < byte_str.size()))
                return -1;
            int tmp = pos;

            while( 0x80 <= byte_str[tmp] & byte_str[tmp] < 0xC0 ){//2byte 以降はこの範囲に入る
                tmp--;
                if(tmp < 0) return -1;
            }
            if(pos != tmp + utf8_bytes(byte_str[tmp]))//assert
                return -1;
            return tmp;
        };//}}}

        int to_unicode(std::vector<unsigned char>& c){//{{{
            int unicode = 0;

            if (c[0] > 0x00ef) { /* 4 bytes */
                return 0; // 無視
            } else if (c[0] > 0xdf) { /* 3 bytes */
                unicode =  (c[0] & 0x0f) << 12;
                unicode += (c[1] & 0x3f) << 6;
                unicode +=  c[2] & 0x3f;
                return unicode;
            } else if (c[0] > 0x7f) { /* 2 bytes */
                unicode = (c[0] & 0x1f) << 6;
                unicode += c[1] & 0x3f;
                return unicode;
            } else { /* 1 byte */
                return c[0];
            }
        };//}}}
            
        inline int utf8_bytes(unsigned char u) {//{{{
            if ( u < 0x80 ) {
                return 1;
            } else if ( 0xe0 <= u & u < 0xf0 ){// よく出てくるので先に調べる
                return 3;
            } else if ( u < 0xe0) {
                return 2;
            } else {
                return 4;
            }
        };//}}}

        int check_code(std::vector<unsigned char>& c) {//{{{
            return check_unicode_char_type(to_unicode(c));
        };//}}}

        // TODO: static const な値として TYPE_... を定義
        unsigned int check_unicode_char_type(int code) {//{{{
            /* SPACE */
            if (code == 0x20 /* space*/|| code == 0x3000 /*全角スペース*/ || 
                    code == 0x00A0 || code == 0x1680 || code == 0x180E ||  /*その他のunicode上のスペース*/
                    (0x2000 >= code && code <= 0x200B )|| code == 0x202F || 
                    code == 0x205F || code == 0xFEFF) {
                return SPACE;
            }
            /* IDEOGRAPHIC PUNCTUATIONS (、。) */
            else if (code > 0x3000 && code < 0x3003) {
                return IDEOGRAPHIC_PUNC;
            }
            else if (0x337B <= code && code <= 0x337E){ /* ㍻㍼㍽㍾ */
                return SYMBOL + ERA;
            }
            /* HIRAGANA */
            else if (code > 0x303f && code < 0x30a0) {
                return HIRAGANA;
            }
            /* KATAKANA and  */
            else if (code > 0x309f && code < 0x30fb ) {
                return KATAKANA;
            }else if ( 0x00A1 <= code && code<= 0x00DF){ //半角カナ(0xA1-0xDF) 
                return HANKAKU_KANA; 
            }else if (code == 0x30fc) { // "ー"(0x30fc)
                return KATAKANA + CHOON;
            }else if (code == 0x30fc) { // "〜"(0x301C)  ⁓ (U+2053)、fullwidth tilde: ～ (U+FF5E)、tilde operator: ∼ (U+223C) 
                return CHOON;
            }
            /* "・"(0x30fb) "· 0x 00B7 */
            else if (code == 0x00B7 || code == 0x30fb ) {
                return MIDDLE_DOT;
            }
            /* "，"(0xff0c), "," (0x002C) */
            else if (code == 0x002C || code == 0xff0c ) {
                return COMMA;
            }
            /* "/"0x002F, "／"(0xff0f) */
            else if (code == 0x002F||code == 0xff0f) {
                return SLASH;
            }
            /* ":" 0x003A "："(0xff1a) */
            else if (code == 0x003A ||code == 0xff1a) {
                return COLON;
            }
            /* PRIOD */
            else if (code == 0xff0e) {
                return PERIOD;
            }
            /* FIGURE (0-9, ０-９) */ //①，ⅷ などはどうするか
            else if ((code > 0x2f && code < 0x3a) || 
                    (code > 0xff0f && code < 0xff1a)) {
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
                    code == 0x5341 || //十
                    code == 0x767e || //百
                    code == 0x5343 || //千
                    code == 0x4e07 || //万
                    code == 0x5104 || //億
                    code == 0x5146 || //兆
                    // 京は数字に含めると副作用が大きそう
                    //code == 0xff05 ||//％
                    // 年月日: code == 0x5e74 || code == 0x6708 || code == 0x65e5 || 
                    //code == 0x4ebf || //??
                    //code == 0x4f59 || //余
                    //code == 0x591a || //多
                    //code == 0x70b9 //点
                    false) {
                        return KANJI_FIGURE;
                    }
            /* ALPHABET (A-Z, a-z, Umlaut etc., Ａ-Ｚ, ａ-ｚ) */
            else if ((code > 0x40 && code < 0x5b) || 
                    (code > 0x60 && code < 0x7b) || 
                    (code > 0xbf && code < 0x0100) || 
                    (code > 0xff20 && code < 0xff3b) || 
                    (code > 0xff40 && code < 0xff5b)) {
                return ALPH;
            }
            /* CJK Unified Ideographs and "々" and "〇"*/
            else if ((code > 0x4dff && code < 0xa000) || code == 0x3005 || code == 0x3007) {
                return KANJI;
            } else {
                return SYMBOL;
            }
        };//}}}

};//}}}

}


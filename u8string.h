
#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
//#include <iterator>
#include <string>
#include <unordered_set>
#include <unordered_map>


/******
 * U8string
 * utf8 の char バイト列を扱うクラス
 * コンストラクタでは utf8 の解釈は行わず，単に内部に std::string としてコピーするのみ．
 * 文字単位の扱いが必要になった時点で，バイト列を解釈し，文字ごとの配列を生成して内部的に用いる．
 * 単に string として扱ってもそれほどオーバーヘッドがない（はず）．
 * string でよく使いそうな機能のみをラップしてある．(find, substr, c_str)
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
    static constexpr unsigned long KATAKANA        =0x00000200; // カタカナ or 長音?
    static constexpr unsigned long HIRAGANA        =0x00000400;
    static constexpr unsigned long KANJI_FIGURE    =0x00000800; // KANJI+FIGURE にするべき？
    static constexpr unsigned long SLASH           =0x00001000;
    static constexpr unsigned long COLON           =0x00002000;
    static constexpr unsigned long ERA             =0x00004000; // ㍻ 
    static constexpr unsigned long CHOON           =0x00008000; // ー, 〜
    static constexpr unsigned long HANKAKU_KANA    =0x00010000; // 半角カタカナ
    static constexpr unsigned long BRACKET         =0x00020000; // 括弧, 引用符
    static constexpr unsigned long FIGURE_EXCEPTION=0x00040000; // 数
    static constexpr unsigned long FIGURE_DIGIT    =0x00080000; // 十，百，千，万，億

    // TYPE_FIGURE + TYPE_PERIOD + TYPE_MIDDLE_DOT + TYPE_KANJI_FIGURE + TYPE_SLASH + TYPE_COLON
    static constexpr unsigned long FAMILY_FIGURE      = 0x00003838; 
    // TYPE_PERIOD + TYPE_COMMA + TYPE_IDEOGRAPHIC_PUNC
    static constexpr unsigned long FAMILY_PUNC        = 0x00000072; 
    // TYPE_ALPH + TYPE_PERIOD + TYPE_SLASH + TYPE_COLON + TYPE_MIDDLE_DOT
    static constexpr unsigned long FAMILY_ALPH_PUNC   = 0x00003090; 
    // TYPE_PERIOD + TYPE_COMMA + TYPE_IDEOGRAPHIC_PUNC + TYPE_MIDDLE_DOT + TYPE_SYMBOL + TYPE_SLASH + TYPE_COLON
    static constexpr unsigned long FAMILY_PUNC_SYMBOL = 0x0000315B; 
    static constexpr unsigned long FAMILY_SPACE       = 0x00000001; // TYPE_SPACE
    static constexpr unsigned long FAMILY_SYMBOL      = 0x00000100; // TYPE_SYMBOL
    static constexpr unsigned long FAMILY_ALPH        = 0x00000080; // TYPE_ALPH
    static constexpr unsigned long FAMILY_KANJI       = 0x0000080A; // TYPE_KANJI + TYPE_KANJI_FIGURE
    static constexpr unsigned long FAMILY_BRACKET     = 0x00020000; // 括弧, 引用符
    static constexpr unsigned long FAMILY_OTHERS      = 0x00000000;

    static const std::unordered_set<std::string> lowercase;
    static const std::unordered_set<unsigned int> brackets; //あとでmap にするかも

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
            if(char_index >= char_size())
                return byte_size();
            return char_index2byte_index[char_index];
        }//}}}

        // i 文字目が何バイトの文字かを表す
        size_t byte_size_at(size_t i){//{{{
            parse();
            return char_array[i].size();
        };//}}}

        unsigned long char_type_at(size_t char_ind){//{{{
            parse();
//            std::cerr << "key= " << byte_str << std::endl;
//            std::cerr << "char_array[" << char_ind <<" ] = " ;
//            for(auto a:char_array[char_ind]){
//                std::cerr << (unsigned long)a << " ";
//            }
//            std::cerr << " " << std::hex << to_unicode(char_array[char_ind]) << std::dec << std::endl;

            if(char_ind >= char_size()) return 0x0000;
            return check_code(char_array[char_ind]);
        }//}}}
        
        // is_katakana, is_lower 等 順次必要になったものを追加予定
        bool is_lower(size_t char_ind){//{{{
            parse();
            return lowercase.find(char_substr(char_ind, 1)) != lowercase.end();
        };//}}}

        bool is_choon(size_t char_ind){//{{{
            parse();
            return (char_type_at(char_ind) & CHOON) > 0;
        };//}}}

        bool is_suuji(size_t char_ind){//{{{
            parse();
            if(char_size() == 0) return false;
            return (char_type_at(char_ind) & (FIGURE|KANJI_FIGURE)) > 0;
        };//}}}
        
        bool is_suuji_digit(size_t char_ind){//{{{
            parse();
            if(char_size() == 0) return false;
            return (char_type_at(char_ind) & (FIGURE_DIGIT)) > 0;
        };//}}}

        bool is_figure_exception(size_t char_ind){//{{{
            parse();
            if(char_size() == 0) return false;
            return (char_type_at(char_ind) & (FIGURE_EXCEPTION)) > 0;
        };//}}}

        bool is_katakana(){//{{{
            parse();
            if(char_size() == 0) return false;
            for(unsigned int i=0;i<char_size();i++){
                if( (char_type_at(i) & (KATAKANA+HANKAKU_KANA)) == 0 )
                    return false;
            }
            return true;
        };//}}}
        
        bool is_kanji(){//{{{
            parse();
            if(char_size() == 0) return false;
            for(unsigned int i=0;i<char_size();i++){
                if( (char_type_at(i) & (KANJI|KANJI_FIGURE)) == 0)
                    return false;
            }
            return true;
        };//}}}

        bool is_eisuu(){//{{{
            parse();
            if(char_size() == 0) return false;
            for(unsigned int i=0;i<char_size();i++){
                if( (char_type_at(i) & (ALPH|FIGURE)) == 0)
                    return false;
            }
            return true;
        };//}}}
        
        
        bool is_alphabet(){//{{{
            parse();
            if(char_size() == 0) return false;
            for(unsigned int i=0;i<char_size();i++){
                if( (char_type_at(i) & (ALPH)) == 0)
                    return false;
            }
            return true;
        };//}}}

        bool is_kigou(){//{{{
            parse();
            if(char_size() == 0) return false;
            for(unsigned int i=0;i<char_size();i++){
                if( (char_type_at(i) & (FAMILY_PUNC_SYMBOL|BRACKET)) == 0)
                    return false;
            }
            return true;
        };//}}}

    private:
        int next(int pos){//{{{
            if(!((0 < pos) & (pos < static_cast<int>(byte_str.size()))))
                return -1;
            int bytes = utf8_bytes(byte_str[pos]);
            if(pos + bytes > static_cast<int>(byte_str.size()))
                return -1;
            return pos + bytes;
        };//}}}

        int prev(int pos){//{{{
            if(!(
                 (0 < pos) & (pos < static_cast<int>(byte_str.size()))
                 ))
                return -1;
            int tmp = pos;

            while( (0x80 <= byte_str[tmp]) & (byte_str[tmp] < 0xC0) ){//2byte 以降はこの範囲に入る
                tmp--;
                if(tmp < 0) return -1;
            }
            if(pos != tmp + utf8_bytes(byte_str[tmp]))//assert
                return -1;
            return tmp;
        };//}}}

        int to_unicode(std::vector<unsigned char>& c);
            
        inline int utf8_bytes(unsigned char u) {//{{{
            if ( u < 0x80 ) {
                return 1;
            } else if ( (0xe0 <= u) & (u < 0xf0) ){// よく出てくるので先に調べる
                return 3;
            } else if ( u < 0xe0) {
                return 2;
            } else {
                return 4;
            }
        };//}}}

        unsigned long check_code(std::vector<unsigned char>& c) {//{{{
            return check_unicode_char_type(to_unicode(c));
        };//}}}

        unsigned long check_unicode_char_type(int code); 

};//}}}

}


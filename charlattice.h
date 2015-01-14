
#ifndef CHARLATTICE_H
#define CHARLATTICE_H

#include <unordered_map>
#include <boost/foreach.hpp>
#include "common.h"
#include "darts.h"

/* 定数 */
#define STOP_MRPH_WEIGHT       255 /* このWeigthのときは形態素候補から除く */
#define OPT_NORMAL             1
#define OPT_NORMALIZE          2
#define OPT_DEVOICE            4
#define OPT_PROLONG_DEL        8
#define OPT_PROLONG_REPLACE    16
#define NORMALIZED_LENGTH      8   /* 非正規表記の処理で考慮する最大形態素長 */

#define	BYTES4CHAR	3	/* UTF-8 (usually) */


//#define MAX_NODE_POS_NUM 10

namespace Morph {

class CharNode{//{{{
    public:
        char chr[7];/// = "";
        char type = 0;

        CharNode *next;
        // おなじ表層で異なるPOS が複数ある場合には，da_node_pos が複数になる．
        std::vector<size_t> da_node_pos;
        //size_t da_node_pos[MAX_NODE_POS_NUM];
        std::vector<char> node_type;
        //string node_type[MAX_NODE_POS_NUM];
        //std::vector<std::string> deleted_bytes;// 形態素のlength の計算時に足していた. 今は不要かも?
        //string deleted_bytes[MAX_NODE_POS_NUM];
        //std::vector<char*> p_buffer;// DAで見つけた形態素の情報をここに書き込んでいた.  今は不要かも?
        //char *p_buffer[MAX_NODE_POS_NUM];
        size_t da_node_pos_num = 0; 

        CharNode(std::string str, int init_type){
            type = init_type;
            //chr = str.copy(chr, sizeof(str));
            str.copy(chr, sizeof(str));
            chr[sizeof(str)] = '\0'; //copy はnull文字を付け加えない
        };
};//}}}

class CharLattice{//{{{
    public:
        std::vector<std::vector<CharNode>> node_list; //node_list[CharNum][replace_index]
        //std::vector<CharNode> root_node; //必要？
        size_t  CharNum;
        int MostDistantPosition;

    public:
        // 文字列を受け取り, lattice に変換する.
        // エラー無し:1 エラー:0 
        int parse(std::string sent);
        std::vector<Darts::DoubleArray::result_pair_type> da_search_one_step(Darts::DoubleArray &da, int left_position, int right_position);
        std::vector<Darts::DoubleArray::result_pair_type> da_search_from_position(Darts::DoubleArray &da, int position);

        CharLattice():CharRootNodeList{CharNode("root", 0)}{};

    public: // クラス定数;

        /* initialization for root node (starting node for looking up double array) */
        std::vector<CharNode> CharRootNodeList;

        /* 小書き文字・拗音(+"ん","ン")、小書き文字に対応する大文字の一覧
           非正規表記の処理(小書き文字・対応する大文字)、
           オノマトペ認識(開始文字チェック、拗音(cf. CONTRACTED_BONUS))で利用 */
        const std::vector<std::string> lowercase{"ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゎ", "ヵ",
            "ァ", "ィ", "ゥ", "ェ", "ォ", "ヮ", "っ", "ッ", "ん", "ン",
            "ゃ", "ャ", "ゅ", "ュ", "ょ", "ョ"};
        const std::vector<std::string> uppercase{"あ", "い", "う", "え", "お", "わ", "か"};
        const std::unordered_map<std::string, std::string> lower2upper{
            {"ぁ", "あ"}, {"ぃ", "い"}, {"ぅ", "う"}, {"ぇ", "え"}, {"ぉ", "お"}, {"ゎ", "わ"}};

        const std::string DEF_PROLONG_SYMBOL1{"ー"};
        const std::string DEF_PROLONG_SYMBOL2{"〜"};
        const std::string DEF_PROLONG_SYMBOL3{"っ"};

        /* 長音置換のルールで利用 */
        // 置き換え先へのmap 場合に応じて使いやすい方で
        const std::unordered_map<std::string, std::string> prolonged_map{
            {"か","あ"}, {"ば","あ"}, {"ま","あ" }, {"ゃ","あ" }, 
            {"い","い"}, {"き","い"}, {"し","い" }, {"ち","い" }, {"に","い" }, {"ひ","い" }, 
            {"じ","い"}, {"け","い"}, {"せ","い" }, {"へ","い" }, {"め","い" }, {"れ","い" }, 
            {"げ","い"}, {"ぜ","い"}, {"で","い" }, {"べ","い" }, {"ぺ","い" }, 
            {"く","う"}, {"す","う"}, {"つ","う" }, {"ふ","う" }, {"ゆ","う" }, {"ぐ","う" }, 
            {"ず","う"}, {"ぷ","う"}, {"ゅ","う" }, {"お","う" }, {"こ","う" }, {"そ","う" }, 
            {"と","う"}, {"の","う"}, {"ほ","う" }, {"も","う" }, {"よ","う" }, {"ろ","う" }, 
            {"ご","う"}, {"ぞ","う"}, {"ど","う" }, {"ぼ","う" }, {"ぽ","う" }, {"ょ","う" }, 
            {"え","え"}, {"ね","え"}}; 

        /* 長音記号直前の文字が pre_prolonged[i] だった場合、長音記号を prolonged2chr[i] に置換 */
        const std::vector<std::string> pre_prolonged{"か", "ば", "ま", "ゃ", /* あ */
            "い", "き", "し", "ち", "に", "ひ", "じ", "け", "せ", /* い */
            "へ", "め", "れ", "げ", "ぜ", "で", "べ", "ぺ",
            "く", "す", "つ", "ふ", "ゆ", "ぐ", "ず", "ぷ", "ゅ", /* う */
            "お", "こ", "そ", "と", "の", "ほ", "も", "よ", "ろ",
            "ご", "ぞ", "ど", "ぼ", "ぽ", "ょ", "え", "ね", "\0"}; /* え(ね) */
        const std::vector<std::string> prolonged2chr{"あ", "あ", "あ", "あ", /* あ */
            "い", "い", "い", "い", "い", "い", "い", "い", "い", /* い */
            "い", "い", "い", "い", "い", "い", "い", "い",
            "う", "う", "う", "う", "う", "う", "う", "う", "う", /* う */
            "う", "う", "う", "う", "う", "う", "う", "う", "う",
            "う", "う", "う", "う", "う", "う", "え", "え", "\0"}; /* え(ね) */
        // ねぇ，ねえ，が除外されている理由は？

        /* 小書き文字削除のルールで利用 */
        /* 長音記号直前の文字が pre_lower[] だった場合、小書き文字を削除 */
        static const unsigned char DELETE_LOWERCASE_S = 0;  /* 削除するlowercase[]の範囲 */
        static const unsigned char DELETE_LOWERCASE_E = 5;  /*   (0,5→"ぁ"から"ぉ"までが対象) */
        const std::vector<int> pre_lower_start{0,  14, 23, 30, 37};
        const std::vector<int> pre_lower_end{14, 23, 30, 37, 45};
        const std::vector<std::string> pre_lower{
            "か", "さ", "た", "な", "は", "ま", "や", "ら", "わ", 
            "が", "ざ", "だ", "ば", "ぱ",                          /* ぁ:14 */
            "い", "し", "に", "り", "ぎ", "じ", "ね", "れ", "ぜ",  /* ぃ: 9 */
            "う", "く", "す", "ふ", "む", "る", "よ",              /* ぅ: 7 */
            "け", "せ", "て", "め", "れ", "ぜ", "で",              /* ぇ: 7 */
            "こ", "そ", "の", "も", "よ", "ろ", "ぞ", "ど"}; /* ぉ: 8 */

        // lower list
        const std::vector<std::string> lower_list{"ぁ", "ぃ", "ぅ", "ぇ", "ぉ"};
        // map 版 内容は同じ
        const std::unordered_map<std::string, std::string> lower_map{
            {"か", "ぁ" }, {"さ", "ぁ" }, {"た", "ぁ" }, {"な", "ぁ" }, {"は", "ぁ" }, 
            {"ま", "ぁ" }, {"や", "ぁ" }, {"ら", "ぁ" }, {"わ", "ぁ" }, {"が", "ぁ" }, 
            {"ざ", "ぁ" }, {"だ", "ぁ" }, {"ば", "ぁ" }, {"ぱ", "ぁ" },
            {"い", "ぃ" }, {"し", "ぃ" }, {"に", "ぃ" }, {"り", "ぃ" }, {"ぎ", "ぃ" }, 
            {"じ", "ぃ" }, {"ね", "ぃ" }, {"れ", "ぃ" }, {"ぜ", "ぃ" },
            {"う", "ぅ" }, {"く", "ぅ" }, {"す", "ぅ" }, {"ふ", "ぅ" }, {"む", "ぅ" }, 
            {"る", "ぅ" }, {"よ", "ぅ" },
            {"け", "ぇ" }, {"せ", "ぇ" }, {"て", "ぇ" }, {"め", "ぇ" }, {"れ", "ぇ" }, 
            {"ぜ", "ぇ" }, {"で", "ぇ" },
            {"こ", "ぉ" }, {"そ", "ぉ" }, {"の", "ぉ" }, {"も", "ぉ" }, {"よ", "ぉ" }, 
            {"ろ", "ぉ" }, {"ぞ", "ぉ" }, {"ど", "ぉ" }
        };
};//}}}

#define COPY_FOR_REFERENCE
#ifndef COPY_FOR_REFERENCE

//とりあえず作ったが 一旦忘れる
class utf8_string{//{{{
    public: 
        std::string str;
       
        size_t size(){
            size_t s = 0;
            auto itr = str.begin();
            auto end = str.end();
            for(auto itr =str.begin();itr != end;;){
                itr+=utf8_bytes(*itr);
                s++;
            }
            return s;
        };

        int next(int pos){
            if(!(0 < pos & pos < str.size()))
                return -1;
            int bytes = utf8_bytes(str[pos]);
            if(pos + bytes > str.size())
                return -1;
            //
            return pos + bytes;
        };

        int prev(int pos){
            if(!(0 < pos & pos < str.size()))
                return -1;
            int tmp = pos;

            while( 0x80 =< str[tmp] & str[tmp] < 0xC0 ){//2byte 以降はこの範囲に入る
                tmp--;
                if(tmp < 0) return -1;
            }
            if(pos != tmp + utf8_bytes(str[tmp]))//assert
                return -1;
            return tmp;
        };
            
    inline int utf8_bytes(unsigned char u) {
        if ( c < 0x80 ) {
            return 1;
        } else if ( 0xe0 =< c & c < 0xf0 ){// よく出てくるので先に調べる
            return 3;
        } else if ( c < 0xe0) {
            return 2;
        } else {
            return 4;
        }
    }

    int check_code(U_CHAR *cp, int pos) {
        auto c = cp[pos];
        auto ucp = cp + pos;
        int unicode;

        if (c == '\0') return 0;
        if (c == KUUHAKU) return KUUHAKU;

        if (c > 0xef) { /* 4 bytes */
            return 0;
        } else if (c > 0xdf) { /* 3 bytes */
            unicode = (c & 0x0f) << 12;
            c = *(ucp + 1);
            unicode += (c & 0x3f) << 6;
            c = *(ucp + 2);
            unicode += c & 0x3f;
            return check_unicode_char_type(unicode);
        } else if (c > 0x7f) { /* 2 bytes */
            unicode = (c & 0x1f) << 6;
            c = *(ucp + 1);
            unicode += c & 0x3f;
            return check_unicode_char_type(unicode);
        } else { /* 1 byte */
            return check_unicode_char_type(c);
        }
    }
}//}}}


// copied from CharRootNode

CHAR_NODE	CharLattice[MAX_LATTICES];
CHAR_NODE	CharRootNode;
size_t		CharNum;

/* 濁音・半濁音、濁音と対応する清音の一覧 
   連濁認識(濁音・対応する清音)、オノマトペ認識(濁音・半濁音(cf. DAKUON_BONUS))で利用 
   (奇数番目を平仮名、続く要素を対応する片仮名とすること) */
U_CHAR *dakuon[] = {"が", "ガ", "ぎ", "ギ", "ぐ", "グ", "げ", "ゲ", "ご", "ゴ",
		    "ざ", "ザ", "じ", "ジ", "ず", "ズ", "ぜ", "ゼ", "ぞ", "ゾ",
		    "だ", "ダ", "ぢ", "ヂ", "づ", "ヅ", "で", "デ", "ど", "ド",
		    "ば", "バ", "び", "ビ", "ぶ", "ブ", "べ", "ベ", "ぼ", "ボ",
		    "ぱ", "パ", "ぴ", "ピ", "ぷ", "プ", "ぺ", "ペ", "ぽ", "ポ", "\0"};
U_CHAR *seion[]  = {"か", "カ", "き", "キ", "く", "ク", "け", "ケ", "こ", "コ",
		    "さ", "サ", "し", "シ", "す", "ス", "せ", "セ", "そ", "ソ",
		    "た", "タ", "ち", "チ", "つ", "ツ", "て", "テ", "と", "ト",
		    "は", "ハ", "ひ", "ヒ", "ふ", "フ", "へ", "ヘ", "ほ", "ホ", "\0"};

/* 小書き文字・拗音(+"ん","ン")、小書き文字に対応する大文字の一覧
   非正規表記の処理(小書き文字・対応する大文字)、
   オノマトペ認識(開始文字チェック、拗音(cf. CONTRACTED_BONUS))で利用 */
U_CHAR *lowercase[] = {"ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゎ", "ヵ",
		       "ァ", "ィ", "ゥ", "ェ", "ォ", "ヮ", "っ", "ッ", "ん", "ン",
		       "ゃ", "ャ", "ゅ", "ュ", "ょ", "ョ", "\0"};
U_CHAR *uppercase[] = {"あ", "い", "う", "え", "お", "わ", "か", "\0"};

/* 長音置換のルールで利用 */
/* 長音記号直前の文字が pre_prolonged[i] だった場合、長音記号を prolonged2chr[i] に置換 */
U_CHAR *pre_prolonged[] = {"か", "ば", "ま", "ゃ", /* あ */
			   "い", "き", "し", "ち", "に", "ひ", "じ", "け", "せ", /* い */
			   "へ", "め", "れ", "げ", "ぜ", "で", "べ", "ぺ",
			   "く", "す", "つ", "ふ", "ゆ", "ぐ", "ず", "ぷ", "ゅ", /* う */
			   "お", "こ", "そ", "と", "の", "ほ", "も", "よ", "ろ",
			   "ご", "ぞ", "ど", "ぼ", "ぽ", "ょ", "え", "ね", "\0"}; /* え(ね) */
U_CHAR *prolonged2chr[] = {"あ", "あ", "あ", "あ", /* あ */
			   "い", "い", "い", "い", "い", "い", "い", "い", "い", /* い */
			   "い", "い", "い", "い", "い", "い", "い", "い",
			   "う", "う", "う", "う", "う", "う", "う", "う", "う", /* う */
			   "う", "う", "う", "う", "う", "う", "う", "う", "う",
			   "う", "う", "う", "う", "う", "う", "え", "え", "\0"}; /* え(ね) */




/* 処理ごとに使用する範囲(指定がない場合、全てを使用) */
#define VOICED_CONSONANT_S      0  /* 連濁認識で使用するdakuon[]の範囲 */
#define VOICED_CONSONANT_E     40  /*   (0,40→"が"から"ボ"までが対象) */
#define NORMALIZED_LOWERCASE_S  0  /* 正規化するlowercase[]の範囲 */
#define NORMALIZED_LOWERCASE_E  7  /*   (0,7→"ぁ"から"ヵ"までが対象) */
#define NORMALIZED_LOWERCASE_KA 6  /* "ヵ"のみ1字からなる形態素を認める(接続助詞「か」) */
#define CONTRACTED_LOWERCASE_S 17  /* 拗音として扱うlowercase[]の範囲 */
#define CONTRACTED_LOWERCASE_E 23  /*   (17,23→"ゃ"から"ョ"までが対象) */

/* 連濁認識用の追加コスト */

/* "が"から始まる動詞を除く動詞の連濁化のコスト
      4以下だと、"盛りだくさん"が正しく解析できない(061031)
      5以下だと、"とこしずめのまつり"が正しく解析できない(070123)
      6以下だと、"カネづるになる"の解釈に不要な曖昧性が生じる(070123)

   "が"から始まる動詞の連濁化のコスト
      7以下だと、"きりがない"が正しく解析できない(060928)
      8以下だと、"疲れがたまる"が正しく解析できない(060928)
     10以上だと、"ひっくりがえす"が正しく解析できない(060928)

   "が"から始まる名詞を除く名詞の連濁化のコスト
      7以下だと、"変わりばえが"の解釈に不要な曖昧性が生じる(060928)
      9以上だと、"ものごころ"が正しく解析できない(060928)

   "が"から始まる名詞の連濁化のコスト
      6以下だと、"右下がりの状態"が正しく解析できない(060928)
     10以下だと、"男がなたで"が正しく解析できない(070123)

   形容詞の連濁化のコスト
     10以上だと、"盛りだくさん"が解析できない(061031) */

#define VERB_VOICED_COST       7  /* "が"から始まる動詞を除く動詞の連濁化のコスト */
#define VERB_GA_VOICED_COST    9  /* "が"から始まる動詞の連濁化のコスト */
#define NOUN_VOICED_COST       8  /* "が"から始まる名詞を除く名詞の連濁化のコスト */
#define NOUN_GA_VOICED_COST   11  /* "が"から始まる名詞の連濁化のコスト */
#define ADJECTIVE_VOICED_COST  9  /* 形容詞の連濁化のコスト */
#define OTHER_VOICED_COST      5  /* 上記以外の連濁化のコスト */

/* 小書き文字を大文字化、平仮名を長音記号に置換する際の追加コスト */
#define NORMALIZED_COST        6

/* 長音を削除する際の追加コスト */
#define PROLONG_DEL_COST1      6  /* 感動詞 */
#define PROLONG_DEL_COST2     50  /* 判定詞 */
#define PROLONG_DEL_COST3      9  /* その他 (9より小さいと"あーあ"を指示詞と解析する) */

/* 反復型オノマトペのコスト */

/* 副詞と認識されるもの
   「ちょろりちょろり」、「ガンガン」、「すべすべ」、「ごうごう」、
   「スゴイスゴイ」、「しゃくしゃく」、「はいはい」、「たらたら」、
   「ぎゅうぎゅう」、「でんでん」、「ギューギュー」、「ガラガラ」、

   副詞と認識されないもの
   「むかしむかし」、「またかまたか」、「もりもり」、「ミニミニ」、
   「さくらさくら」、「おるおる」、「いるいる」、「あったあった」、
   「とべとべ」、「ごめんごめん」、「とぎれとぎれ」、「ジャジャ」 
   「ぜひぜひ」 */
   

#define REPETITION_COST       13  /* 繰り返し1音あたりのコスト */
#define DAKUON_BONUS           1  /* 濁音があった場合のボーナス */
#define CONTRACTED_BONUS       4  /* 拗音があった場合のボーナス */
#define KATAKANA_BONUS         2  /* カタカナであった場合のボーナス */

/* 非反復型オノマトペ認識用の定数 */
#define PATTERN_MAX      64

#ifdef IO_ENCODING_EUC
#define Hcode            "\xA4[\xA2-\xEF]"
#define Kcode            "\xA5[\xA1-\xF4]"
#define Ycode            "[\xA4\xA5][\xE3\xE5\xE7]"
#else /* UTF-8 */
#define Hcode            "\xE3(\x81[\x82-\xBF]|\x82[\x80-\x8F])"
#define Kcode            "\xE3(\x82[\xA0-\xBF]|\x83[\x80-\xBA])"
#define Ycode            "\xE3(\x82[\x83\x85\x87]|\x83[\xA3\xA5\xA7])"
#endif

#define Hkey             "Ｈ"
#define Ykey             "Ｙ"
#define Kkey             "Ｋ"
#define DefaultWeight    10

#ifdef HAVE_REGEX_H
/* 非反復型オノマトペのパターンを保持するための構造体 */
typedef struct {
    char        regex[PATTERN_MAX];
    regex_t     preg;
    double      weight;
} MRPH_PATTERN;
MRPH_PATTERN *m_pattern;

/* 非反復型オノマトペのパターンとコスト */
/* 書式: 【パターン コスト】
   平仮名[あーわ]にマッチさせる場合は"Ｈ", ("ぁ", "ゐ", "ゑ", "を", "ん"は含まないので注意)
     片仮名[ァーヴ]にマッチさせる場合は"Ｋ", 
     [ゃゅょャュョ]にマッチさせる場合は"Ｙ"と記述
   高速化のため、
     通常の平仮名、片仮名以外から始まるもの、
     1文字目と2文字目の文字種が同じものに限定 */
char *mrph_pattern[]  = {
    "ＨっＨり    30", /* もっさり */
    "ＨっＨＹり  30", /* ぐっちょり */
    "ＫッＫリ    30", /* モッサリ */
    "ＫッＫＹリ  30", /* ズッチョリ */
    "ＨＨっと    24", /* かりっと */
    "ＫＫっと    20", /* ピタっと */
    "ＫＫッと    20", /* ピタッと */
    "\0"};
#endif


    
//*/
//
//
void char_lattice_sample(char* String, int length){//{{{

    /* initialization for root node (starting node for looking up double array) */
    CharRootNode.next = NULL;
    CharRootNode.da_node_pos[0] = 0;
    CharRootNode.deleted_bytes[0] = 0;
    CharRootNode.p_buffer[0] = NULL;
    CharRootNode.da_node_pos_num = 1;

    //CharLatticeUsedFlag = FALSE;
    CharNum = 0;
    for (pos = 0; pos < length; pos+=next_pos) {
        current_char_node = &CharLattice[CharNum];
        if (String[pos]&0x80) { /* 全角の場合 */
            /* 長音記号による置換 */
            if (LongSoundRep_Opt &&
                    (!strncmp(String + pos, DEF_PROLONG_SYMBOL1, BYTES4CHAR) ||
                     !strncmp(String + pos, DEF_PROLONG_SYMBOL2, BYTES4CHAR) && 
                     (!String[pos + BYTES4CHAR] || check_code(String, pos + BYTES4CHAR) == KIGOU || check_code(String, pos + BYTES4CHAR) == HIRAGANA)) &&
                    (pos > 0) /* 2文字目以降 */
                    /* 次の文字が"ー","〜"でない */
                    /*  strncmp(String + pos + BYTES4CHAR, DEF_PROLONG_SYMBOL1, BYTES4CHAR) && */
                    /*  strncmp(String + pos + BYTES4CHAR, DEF_PROLONG_SYMBOL2, BYTES4CHAR)) */
                ) {
                    for (i = 0; *pre_prolonged[i]; i++) {
                        // 長音を母音に置換してCHAR_NODE つくり，current_char_node につなげる
                        if (!strncmp(String + pos - pre_byte_length, pre_prolonged[i], pre_byte_length)) {
                            new_char_node = make_new_node(&current_char_node, prolonged2chr[i], OPT_NORMALIZE | OPT_PROLONG_REPLACE);
                            break;
                        }
                    }
                }
            /* 小書き文字による置換 */
            else if (LowercaseRep_Opt) {
                for (i = NORMALIZED_LOWERCASE_S; i < NORMALIZED_LOWERCASE_E; i++) {//置き換えられる文字一覧をチェック
                    if (!strncmp(String + pos, lowercase[i], BYTES4CHAR)) {
                        new_char_node = make_new_node(&current_char_node, uppercase[i], OPT_NORMALIZE);
                        break;
                    }
                }
            }

            next_pos = utf8_bytes(String + pos);
        } else {
            next_pos = 1;
        }
        // 置き換え挿入の無い普通のコピー
        strncpy(CharLattice[CharNum].chr, String + pos, next_pos);
        CharLattice[CharNum].chr[next_pos] = '\0';
        CharLattice[CharNum].type = OPT_NORMAL;
        next_pre_is_deleted = 0;

        /* 長音挿入 (の削除)*/
        if ((LongSoundDel_Opt || LowercaseDel_Opt) && next_pos == BYTES4CHAR) {
            pre_code = (pos > 0) ? check_code(String, pos - pre_byte_length) : -1;
            post_code = check_code(String, pos + next_pos); /* 文末の場合は0 */
            if (LongSoundDel_Opt && pre_code > 0 && (WORD_CHAR_NUM_MAX + local_deleted_num + 1) * BYTES4CHAR < MIDASI_MAX &&
                    /* 直前が削除された長音記号、平仮名、または、漢字かつ直後が平仮名 */
                    ((pre_is_deleted ||
                      pre_code == HIRAGANA || pre_code == KANJI && post_code == HIRAGANA) &&
                     /* "ー"または"〜" */
                     (!strncmp(String + pos, DEF_PROLONG_SYMBOL1, BYTES4CHAR) ||
                      !strncmp(String + pos, DEF_PROLONG_SYMBOL2, BYTES4CHAR))) ||
                    /* 直前が長音記号で、現在文字が"っ"、かつ、直後が文末、または、記号の場合も削除 */
                    (pre_is_deleted && !strncmp(String + pos, DEF_PROLONG_SYMBOL3, BYTES4CHAR) &&
                     (post_code == 0 || post_code == KIGOU))) {
                local_deleted_num++;
                next_pre_is_deleted = 1;
                new_char_node = make_new_node(&current_char_node, "", OPT_PROLONG_DEL);
            }
            else if (LowercaseDel_Opt && pre_code > 0 && (WORD_CHAR_NUM_MAX + local_deleted_num + 1) * BYTES4CHAR < MIDASI_MAX) {
                /* 小書き文字による長音化 */
                for (i = DELETE_LOWERCASE_S; i < DELETE_LOWERCASE_E; i++) { //"ぁ~ぉ"のインデックス
                    if (!strncmp(String + pos, lowercase[i], BYTES4CHAR)) { //いま見ている文字が "ぁぃぅぇぉ"のいずれか(strncmp は一致すると偽(0))
                        for (j = pre_lower_start[i]; j < pre_lower_end[i]; j++) { //ぁぃぅぇぉに対応する"かさたなはまや〜"の範囲
                            // 一文字前が, 対応する文字("ぁ"に対する"か"など)
                            if (!strncmp(String + pos - pre_byte_length, pre_lower[j], pre_byte_length)) break;
                        }
                        /* 直前の文字が対応する平仮名、 ("かぁ" > "か(ぁ)" :()内は削除の意味)
                         * または、削除された同一の小書き文字の場合は削除 ("か(ぁ)ぁ" > "か(ぁぁ)") */
                        if (j < pre_lower_end[i] ||
                                pre_is_deleted && !strncmp(String + pos - pre_byte_length, String + pos, pre_byte_length)) {
                            local_deleted_num++; // 削除した文字数のカウント(これは何のため？)
                            next_pre_is_deleted = 1;
                            // 文字を削除する場合は ""(\0) をchr に入れておく
                            new_char_node = make_new_node(&current_char_node, "", OPT_PROLONG_DEL);
                            break;
                        }
                    }
                }
            }
            else {
                local_deleted_num = 0;
            }
        }
        pre_is_deleted = next_pre_is_deleted;
        pre_byte_length = next_pos;
        current_char_node->next = NULL;
        CharNum++;
    }
}//}}}

CHAR_NODE *make_new_node(CHAR_NODE **current_char_node_ptr, char *chr, int type)
{
    int i;
    CHAR_NODE *new_char_node = (CHAR_NODE *)malloc(sizeof(CHAR_NODE));
    strcpy(new_char_node->chr, chr);
    new_char_node->type = type;
    new_char_node->da_node_pos_num = 0;
    for (i = 0; i < MAX_NODE_POS_NUM; i++)
        new_char_node->p_buffer[i] = NULL;
    (*current_char_node_ptr)->next = new_char_node;
    *current_char_node_ptr = new_char_node;
    CharLatticeUsedFlag = TRUE;
    return new_char_node;
}


CHAR_NODE *make_new_node(CHAR_NODE **current_char_node_ptr, char *chr, int type)
{
    int i;
    CHAR_NODE *new_char_node = (CHAR_NODE *)malloc(sizeof(CHAR_NODE));
    strcpy(new_char_node->chr, chr);
    new_char_node->type = type;
    new_char_node->da_node_pos_num = 0;
    for (i = 0; i < MAX_NODE_POS_NUM; i++)
        new_char_node->p_buffer[i] = NULL;
    // 一つ前のchar node のnext に追加 < わかる
    (*current_char_node_ptr)->next = new_char_node;
    // １つ前のchar node のポインタ上書き ？
    *current_char_node_ptr = new_char_node;
    CharLatticeUsedFlag = TRUE;
    // 値で返したら，コピーなので next との整合性が取れないのでは？// (使ってなかった)
    return new_char_node;
}

#endif
}

#endif


#ifndef CHARLATTICE_H
#define CHARLATTICE_H

#include <unordered_map>
#include <unordered_set>
#include "common.h"
#include "darts.h"

/* 定数 */
#define STOP_MRPH_WEIGHT       255 /* このWeigthのときは形態素候補から除く */
#define OPT_NORMAL             1
#define OPT_NORMALIZE          2
#define OPT_DEVOICE            4
#define OPT_PROLONG_DEL        8
#define OPT_PROLONG_REPLACE    16
#define OPT_PROLONG_DEL_LAST   32
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
        //std::vector<int> result; //da を引いたstates を保存
        //string deleted_bytes[MAX_NODE_POS_NUM];
        //std::vector<char*> p_buffer;// DAで見つけた形態素の情報をここに書き込んでいた.  今は不要かも?
        //char *p_buffer[MAX_NODE_POS_NUM];
        size_t da_node_pos_num = 0; 

        CharNode(std::string str, int init_type){
            for(auto&c:chr)c='\0';
            type = init_type;
            //chr = str.copy(chr, sizeof(str));
            size_t str_size = str.size();
            if(str_size > 7) return;
            str.copy(chr, str_size); //一文字でない場合に例外を返せないので，コンストラクタで作るべきではない？
            chr[str_size] = '\0'; //copy はnull文字を付け加えない
        };
};//}}}

class CharLattice{//{{{
    private:
        bool constructed = false;
    public:

        //typedef Darts::DoubleArray::result_pair_type da_result_pair_type;
        typedef std::tuple<Darts::DoubleArray::value_type, size_t, unsigned int> da_result_pair_type;
        std::vector<std::vector<CharNode>> node_list; //node_list[CharNum][replace_index]
        //std::vector<CharNode> root_node; //必要？
        size_t  CharNum;
        int MostDistantPosition;
        std::vector<size_t> char_byte_length;

    public:
        // 文字列を受け取り, lattice に変換する.
        // エラー無し:1 エラー:0 
        int parse(std::string sent);
        std::vector<CharLattice::da_result_pair_type> da_search_one_step(Darts::DoubleArray &da, int left_position, int right_position);
        std::vector<CharLattice::da_result_pair_type> da_search_from_position(Darts::DoubleArray &da, int position);
            
        CharLattice():CharRootNodeList{CharNode("root", 0)}{
            CharRootNodeList.back().da_node_pos_num = 1;
            CharRootNodeList.back().da_node_pos.push_back(0);
            CharRootNodeList.back().node_type.push_back(0);//初期値は0で良い？
        };

    public: // クラス定数;

        /* initialization for root node (starting node for looking up double array) */
        std::vector<CharNode> CharRootNodeList;

        /* 小書き文字・拗音(+"ん","ン")、小書き文字に対応する大文字の一覧
           非正規表記の処理(小書き文字・対応する大文字)、
           オノマトペ認識(開始文字チェック、拗音(cf. CONTRACTED_BONUS))で利用 */
        const std::unordered_set<std::string> lowercase{"ぁ", "ぃ", "ぅ", "ぇ", "ぉ", "ゎ", "ヵ",
            "ァ", "ィ", "ゥ", "ェ", "ォ", "ヮ", "っ", "ッ", "ん", "ン",
            "ゃ", "ャ", "ゅ", "ュ", "ょ", "ョ"};
        const std::vector<std::string> uppercase{"あ", "い", "う", "え", "お", "わ", "か"};
        const std::unordered_map<std::string, std::string> lower2upper{
            {"ぁ", "あ"}, {"ぃ", "い"}, {"ぅ", "う"}, {"ぇ", "え"}, {"ぉ", "お"}, {"ゎ", "わ"},{"ヶ","ケ"},{"ケ","ヶ"}};

        const std::string DEF_PROLONG_SYMBOL1{"ー"};
        const std::string DEF_PROLONG_SYMBOL2{"〜"};
        const std::string DEF_PROLONG_SYMBOL3{"っ"};
        const std::string DEF_ELIPSIS_SYMBOL1{"…"};//HORIZONTAL ELLIPSIS
        const std::string DEF_ELIPSIS_SYMBOL2{"⋯"};//MIDDLE_HORIZONTAL ELLIPSIS

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
        const std::unordered_set<std::string> lower_list{"ぁ", "ぃ", "ぅ", "ぇ", "ぉ"};
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


}// namespace Morph

#endif

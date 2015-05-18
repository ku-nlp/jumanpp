#include "common.h"
#include "feature.h"
#include "node.h"

namespace Morph {

int Node::id_count = 1;
DBM_FILE Node::topic_cdb;
Parameter *Node::param;

const char* hukugouji[] = { "めぐる", "除く", "のぞく", "通じる", "通ずる", "通す", "含める", "ふくめる", "始める", "はじめる", "絡む", "からむ", "沿う", "そう", "向ける", "伴う", "ともなう", "基づく", "よる", "対する", "関する", "代わる", "おく", "つく", "とる", "加える", "くわえる", "限る", "続く", "つづく", "合わせる", "あわせる", "比べる", "くらべる", "並ぶ", "ならぶ", "おける", "いう", "する" };

Node::Node():bq(param->N) {//{{{
    this->id = id_count++;
    if(topic_cdb == nullptr){
        topic_cdb = db_read_open(cdb_filename);
    }
}//}}}

bool Node::is_dummy(){//{{{
    return (stat == MORPH_DUMMY_POS || stat == MORPH_BOS_NODE || stat ==  MORPH_EOS_NODE); 
};//}}}

//Node::Node(const Node& node) { // 普通のコピー//{{{
//    
//    this->prev = node.prev;
//    this->next = node.next;
//    this->enext = node.enext;
//    this->bnext = node.bnext;
//    this->surface = node.surface;
//    this->representation = node.representation;
//    this->semantic_feature = node.semantic_feature; 
//    this->debug_info = node.debug_info;
//    this->length = node.length; /* length of morph */
//    this->char_num = node.char_num;
//    this->rcAttr = node.rcAttr;
//    this->lcAttr = node.lcAttr;
//	this->posid = node.posid;
//	this->sposid = node.sposid;
//	this->formid = node.formid;
//	this->formtypeid = node.formtypeid;
//	this->baseid = node.baseid;
//	this->repid = node.repid;
//	this->imisid = node.imisid;
//	this->readingid = node.readingid;
//    this->pos = node.pos;
//	this->spos = node.spos;
//	this->form = node.form;
//	this->form_type = node.form_type;
//	this->base = node.base;
//    this->reading = node.reading;
//    this->char_type = node.char_type;
//    this->char_family = node.char_family;
//    this->end_char_family = node.end_char_family;
//    this->stat = node.stat;
//    this->used_in_nbest = node.used_in_nbest;
//    this->wcost = node.wcost;
//    this->cost = node.cost;
//    this->token = node.token;
//
//	//for N-best and Juman-style output
//	this->id = node.id;
//	this->starting_pos = node.starting_pos; 
//
//    if (node.string_for_print)
//        this->string_for_print = new std::string(*node.string_for_print);
//    if (node.end_string)
//        this->end_string = new std::string(*node.end_string);
//    if (node.string)
//        this->string = new std::string(* node.string);
//    if (node.original_surface)
//        this->original_surface = new std::string(* node.original_surface);
//    if (node.feature)
//        this->feature = new FeatureSet(*node.feature);
//}//}}}

Node::~Node() {//{{{
    if (string)
        delete string;
    if (string_for_print)
        delete string_for_print;
    if (end_string)
        delete end_string;
    if (original_surface)
        delete original_surface;
    if (feature)
        delete feature;
}//}}}

void Node::clear(){//{{{
    if (string)
        delete string;
    if (string_for_print)
        delete string_for_print;
    if (end_string)
        delete end_string;
    if (original_surface)
        delete original_surface;
    if (feature)
        delete feature;
    //*this = Node();
}//}}}

void Node::print() {//{{{
    cout << *(string_for_print) << "_" << *pos << ":" << *spos;
}//}}}

std::string Node::str() {//{{{
    return *(string_for_print) + "_" + *pos + ":" + *spos;
}//}}}

const char *Node::get_first_char() {//{{{
    return string_for_print->c_str();
}//}}}

unsigned short Node::get_char_num() {//{{{
    if (char_num >= MAX_RESOLVED_CHAR_NUM)
        return MAX_RESOLVED_CHAR_NUM;
    else
        return char_num;
}//}}}

bool Node::topic_available(){//{{{
    Node* node = this;
    if(!node->representation) return false;

    // 複合辞の処理
    // めぐる 除く のぞく 通じる 通ずる 通す 含める ふくめる 始める はじめる 絡む からむ 沿う そう 向ける 伴う ともなう 基づく よる 対する 関する 代わる おく つく とる 加える くわえる 限る 続く つづく 合わせる あわせる 比べる くらべる 並ぶ ならぶ おける いう する
    int not_hukugou_ji = 1;
    for( int j=0; j< NUM_OF_FUKUGOUJI;j++ ){
        // 代表表記の前半と複合辞が一致しない
        not_hukugou_ji = not_hukugou_ji && node->representation && (node->representation->compare(0, strlen(hukugouji[j]), hukugouji[j], strlen(hukugouji[j])));
    }
        
    int not_huzoku = (node->semantic_feature) && (node->semantic_feature->find("付属動詞候補") == std::string::npos); //付属動詞でない
        
    int not_keisiki = !((*node->pos == "名詞")&&(*node->spos == "形式名詞"));// 形式名詞でない
        
    int not_timei_jinmei = (!((*node->pos == "名詞")&&(*node->spos == "人名")) && 
                           !((*node->pos == "名詞")&&(*node->spos == "地名")));
        
    int not_jisou = !((*node->pos == "名詞")&&(*node->spos == "時相名詞"));// 時相名詞でない
    int not_suuryou = !((*node->pos == "名詞")&&(*node->spos == "数詞"));
    int focused_hinsi = !(*node->pos == ("特殊")|| //1
                        // 動詞 2
                        // 形容詞 3
                        *node->pos == "判定詞"|| //4
                        *node->pos == "助動詞"|| //5
                        // 名詞 6
                        *node->pos == "指示詞"|| //7
                        // 副詞 8
                        *node->pos == "助詞"  || //9
                        *node->pos == "接続詞"|| //10
                        *node->pos == "連体詞"|| //11
                        // 感動詞 12
                        *node->pos == "接頭辞"|| //13
                        *node->pos == "接尾辞"|| //14
                        // mrph->hinsi == 5); //助動詞
                        // 未定義語 15
                        0);
        
    // 漢字一字，ひらがな一字, 記号にはトピックが関係ない場合が多いはずなので，一文字の形態素はトピックを考えない．
    bool not_one_char = (node->char_num > 1); //表層が日本語1文字以上
    char* result = db_get(topic_cdb, node->base->c_str()); // TODO:二回トピックを引くのは無駄なので，キャッシュする
    // トピックを読み込む条件
    bool topical_cond = not_one_char && focused_hinsi && not_huzoku && not_suuryou && not_hukugou_ji && not_keisiki && not_timei_jinmei && not_jisou && result;

    if(result)
        delete(result);

    return topical_cond;
}//}}}

bool Node::topic_available_for_sentence() {//{{{
    //Node* node = this;
    // トピックを読み込む条件
    bool topical_cond = topic_available();
        
    // 文のトピックに入れるかどうかの条件 (現状, topic_available() と同じ)
    // nbest 中に別の読みがあるかどうかは，ここではチェックできない
    return (topical_cond);
}//}}}

void Node::read_vector(const char* buf, std::vector<double> &vector) { //topic用 {{{
    char *copy = strdup(buf);  
    char *copy_str = copy;
    char *token;
    char *saveptr1;
    int i;
    double sum=0.0;
        
    vector.resize(TOPIC_NUM);
    for (i = 0; i < TOPIC_NUM ; i++, copy=NULL){
        token = strtok_r(copy, " ", &saveptr1 );
        if (token == NULL)
            break;
        sscanf(token, "%lf", &(vector[i]));
        // テンプレ for all_uniq_50
        if (i == 1 || // テンプレート
            i == 17|| // ヘッダ・フッダ
            i == 27|| // ブログテンプレ
            i == 35|| // テンプレ？
            i == 46|| // テンプレ
            i == 48|| // アフィリエイト
            false ){
            vector[i] = 0.0;
        }
        sum += vector[i]*vector[i];
    }
    sum = sqrt(sum);
    if(sum == 0.0){
        sum = TOPIC_NUM;
    }
    // 正規化
    for (i = 0; i < TOPIC_NUM ; i++, copy=NULL){
        vector[i] = vector[i] / sum;
    }
    free(copy_str);
}//}}}

TopicVector Node::get_topic(){//{{{
    TopicVector node_topic;
    node_topic.resize(TOPIC_NUM, 0.0);
    if(!representation)
        return node_topic;
    //std::cout << * representation << std::endl;
    const char* result = db_get(topic_cdb, base->c_str()); // topic ベクトルの読み込み
    if(!result){ return node_topic; }//トピックが無い場合

    //std::cout << result << std::endl;
    read_vector(result, node_topic); 
    if(result)
        delete result;
    return node_topic;
}//}}}

}

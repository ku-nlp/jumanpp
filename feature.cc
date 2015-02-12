#include "common.h"
#include "pos.h"
#include "sentence.h"
#include "feature.h"

namespace Morph {

DBM_FILE FeatureSet::topic_cdb;
char* hukugouji[] = { "めぐる", "除く", "のぞく", "通じる", "通ずる", "通す", "含める", "ふくめる", "始める", "はじめる", "絡む", "からむ", "沿う", "そう", "向ける", "伴う", "ともなう", "基づく", "よる", "対する", "関する", "代わる", "おく", "つく", "とる", "加える", "くわえる", "限る", "続く", "つづく", "合わせる", "あわせる", "比べる", "くらべる", "並ぶ", "ならぶ", "おける", "いう", "する" };

FeatureSet::FeatureSet(FeatureTemplateSet *in_ftmpl) {
	ftmpl = in_ftmpl;
    if(topic_cdb == nullptr){
        topic_cdb = db_read_open(cdb_filename);
         
    }
}

FeatureSet::~FeatureSet() {
	fset.clear();
}

void FeatureSet::extract_unigram_feature(Node *node) {//{{{
    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
        if (!((*tmpl_it)->get_is_unigram())) // skip bigram feature template
            continue;
        std::string f = (*tmpl_it)->get_name() + ":";
        for (std::vector<unsigned int>::iterator it = (*tmpl_it)->get_features()->begin(); it != (*tmpl_it)->get_features()->end(); it++) {
            if (it != (*tmpl_it)->get_features()->begin())
                f += ",";
            if (*it == FEATURE_MACRO_WORD)
                f += *(node->string);
            else if (*it == FEATURE_MACRO_POS)
                f += *(node->pos);
            else if (*it == FEATURE_MACRO_SPOS) //品詞細分類
                f += *(node->spos);
            else if (*it == FEATURE_MACRO_FORM) //活用形
                f += *(node->form);
            else if (*it == FEATURE_MACRO_FORM_TYPE) //活用型
                f += *(node->form_type);
            else if (*it == FEATURE_MACRO_FUNCTIONAL_WORD) {//機能語
                if( *(node->pos) == "助詞" || *(node->pos) == "助動詞" || *(node->pos) == "判定詞")
                    f += *(node->string) + *(node->pos) + *(node->spos);
                else
                    f += *(node->pos);
            }
            else if (*it == FEATURE_MACRO_BASE_WORD) //原型
                f += *(node->base);
            else if (*it == FEATURE_MACRO_LENGTH)
                f += int2string(node->get_char_num());
            else if (*it == FEATURE_MACRO_BEGINNING_CHAR)
                f.append(node->get_first_char(), (node->stat & MORPH_PSEUDO_NODE) ? strlen(node->get_first_char()) : utf8_bytes((unsigned char *)node->get_first_char()));
            else if (*it == FEATURE_MACRO_ENDING_CHAR) {
                f += *(node->end_string);
            }
            else if (*it == FEATURE_MACRO_BEGINNING_CHAR_TYPE)
                f += int2string(node->char_family);
            else if (*it == FEATURE_MACRO_ENDING_CHAR_TYPE)
                f += int2string(node->end_char_family);
            else if (*it == FEATURE_MACRO_FEATURE1) // Wikipedia (test)
                f += int2string(node->lcAttr);
        }
        fset.push_back(f);
    }
}//}}}

void FeatureSet::extract_bigram_feature(Node *l_node, Node *r_node) {//{{{
    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
        if ((*tmpl_it)->get_is_unigram()) // skip unigram feature template
            continue;
        std::string f = (*tmpl_it)->get_name() + ":";
        for (std::vector<unsigned int>::iterator it = (*tmpl_it)->get_features()->begin(); it != (*tmpl_it)->get_features()->end(); it++) {
            if (it != (*tmpl_it)->get_features()->begin())
                f += ",";
            // left
            if (*it == FEATURE_MACRO_LEFT_WORD)
                f += *(l_node->string);
            else if (*it == FEATURE_MACRO_LEFT_POS)
                f += *(l_node->pos);
            else if (*it == FEATURE_MACRO_LEFT_SPOS)
                f += *(l_node->spos);
            else if (*it == FEATURE_MACRO_LEFT_FORM)
                f += *(l_node->form);
            else if (*it == FEATURE_MACRO_LEFT_FORM_TYPE)
                f += *(l_node->form_type);
            else if (*it == FEATURE_MACRO_LEFT_FUNCTIONAL_WORD){
                if( *(l_node->pos) == "助詞" || *(l_node->pos) == "助動詞" || *(l_node->pos) == "判定詞")
                    f += *(l_node->string) + ":" + *(l_node->pos) + ":"+ *(l_node->spos);
                else
                    f += *(l_node->pos);
            }
            else if (*it == FEATURE_MACRO_LEFT_BASE_WORD) //原型
                f += *(l_node->base);
            else if (*it == FEATURE_MACRO_LEFT_LENGTH)
                f += int2string(l_node->get_char_num());
            else if (*it == FEATURE_MACRO_LEFT_BEGINNING_CHAR)
                f.append(l_node->get_first_char(), (l_node->stat & MORPH_PSEUDO_NODE) ? strlen(l_node->get_first_char()) : utf8_bytes((unsigned char *)l_node->get_first_char()));
            else if (*it == FEATURE_MACRO_LEFT_ENDING_CHAR) {
                f += *(l_node->end_string);
                // cerr << *(l_node->string) << " : " << *(l_node->string_for_print) << " : " << f << endl;
            }
            else if (*it == FEATURE_MACRO_LEFT_BEGINNING_CHAR_TYPE)
                f += int2string(l_node->char_family);
            else if (*it == FEATURE_MACRO_LEFT_ENDING_CHAR_TYPE)
                f += int2string(l_node->end_char_family);
            // right
            else if (*it == FEATURE_MACRO_RIGHT_WORD)
                f += *(r_node->string);
            else if (*it == FEATURE_MACRO_RIGHT_POS)
                f += *(r_node->pos);
            else if (*it == FEATURE_MACRO_RIGHT_SPOS)
                f += *(r_node->spos);
            else if (*it == FEATURE_MACRO_RIGHT_FORM)
                f += *(r_node->form);
            else if (*it == FEATURE_MACRO_RIGHT_FORM_TYPE)
                f += *(r_node->form_type);
            else if (*it == FEATURE_MACRO_RIGHT_FUNCTIONAL_WORD)
                if( *(r_node->pos) == "助詞" || *(r_node->pos) == "助動詞" || *(r_node->pos) == "判定詞")
                    f += *(r_node->string) + ":" + *(r_node->pos) + ":"+ *(r_node->spos);
                else
                    f += *(r_node->pos);
            else if (*it == FEATURE_MACRO_RIGHT_BASE_WORD) //原型
                f += *(r_node->base);
            else if (*it == FEATURE_MACRO_RIGHT_LENGTH)
                f += int2string(r_node->get_char_num());
            else if (*it == FEATURE_MACRO_RIGHT_BEGINNING_CHAR)
                f.append(r_node->get_first_char(), (r_node->stat & MORPH_PSEUDO_NODE) ? strlen(r_node->get_first_char()) : utf8_bytes((unsigned char *)r_node->get_first_char()));
            else if (*it == FEATURE_MACRO_RIGHT_ENDING_CHAR)
                f += *(r_node->end_string);
            else if (*it == FEATURE_MACRO_RIGHT_BEGINNING_CHAR_TYPE)
                f += int2string(r_node->char_family);
            else if (*it == FEATURE_MACRO_RIGHT_ENDING_CHAR_TYPE)
                f += int2string(r_node->end_char_family);
        }
        fset.push_back(f);
    }
}//}}}

bool FeatureSet::append_feature(FeatureSet *in) {//{{{
    for (std::vector<std::string>::iterator it = in->fset.begin(); it != in->fset.end(); it++) {
        fset.push_back(*it);
    }
    return true;
}//}}}

double FeatureSet::calc_inner_product_with_weight() {//{{{
    double sum = 0;
    for (std::vector<std::string>::iterator it = fset.begin(); it != fset.end(); it++) {
        if (feature_weight.has_key(*it)) {
            sum += feature_weight[*it];
        }
    }
    return sum;
}//}}}

void FeatureSet::minus_feature_from_weight(std::unordered_map<std::string, double> &in_feature_weight, size_t factor) {//{{{
    for (std::vector<std::string>::iterator it = fset.begin(); it != fset.end(); it++) {
        in_feature_weight[*it] -= factor;
    }
}//}}}

void FeatureSet::minus_feature_from_weight(std::unordered_map<std::string, double> &in_feature_weight) {//{{{
    minus_feature_from_weight(in_feature_weight, 1);
}//}}}

void FeatureSet::plus_feature_from_weight(std::unordered_map<std::string, double> &in_feature_weight, size_t factor) {//{{{
    for (std::vector<std::string>::iterator it = fset.begin(); it != fset.end(); it++) {
        in_feature_weight[*it] += factor;
    }
}//}}}

void FeatureSet::plus_feature_from_weight(std::unordered_map<std::string, double> &in_feature_weight) {//{{{
    plus_feature_from_weight(in_feature_weight, 1);
}//}}}

bool FeatureSet::print() {//{{{
    for (std::vector<std::string>::iterator it = fset.begin(); it != fset.end(); it++) {
        cerr << *it << " ";
    }
    cerr << endl;
    return true;
}//}}}

FeatureTemplate::FeatureTemplate(std::string &in_name, std::string &feature_string, bool in_is_unigram) {//{{{
    is_unigram = in_is_unigram;
    name = in_name;
    std::vector<std::string> line;
    split_string(feature_string, ",", line);
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++) {
        unsigned int macro_id = interpret_macro(*it);
        if (macro_id)
            features.push_back(macro_id);
    }
}//}}}

unsigned int FeatureTemplate::interpret_macro(std::string &macro) {//{{{
    // unigram
    if (is_unigram) {
        if (macro == FEATURE_MACRO_STRING_WORD)
            return FEATURE_MACRO_WORD;
        else if (macro == FEATURE_MACRO_STRING_POS)
            return FEATURE_MACRO_POS;
		else if (macro == FEATURE_MACRO_STRING_SPOS)
			return FEATURE_MACRO_SPOS;
		else if (macro == FEATURE_MACRO_STRING_FORM)
			return FEATURE_MACRO_FORM;
		else if (macro == FEATURE_MACRO_STRING_FORM_TYPE)
			return FEATURE_MACRO_FORM_TYPE;
		else if (macro == FEATURE_MACRO_STRING_FUNCTIONAL_WORD)
			return FEATURE_MACRO_FUNCTIONAL_WORD;
        else if (macro == FEATURE_MACRO_STRING_LENGTH)
            return FEATURE_MACRO_LENGTH;
        else if (macro == FEATURE_MACRO_STRING_BEGINNING_CHAR)
            return FEATURE_MACRO_BEGINNING_CHAR;
        else if (macro == FEATURE_MACRO_STRING_ENDING_CHAR)
            return FEATURE_MACRO_ENDING_CHAR;
        else if (macro == FEATURE_MACRO_STRING_BEGINNING_CHAR_TYPE)
            return FEATURE_MACRO_BEGINNING_CHAR_TYPE;
        else if (macro == FEATURE_MACRO_STRING_ENDING_CHAR_TYPE)
            return FEATURE_MACRO_ENDING_CHAR_TYPE;
        else if (macro == FEATURE_MACRO_STRING_FEATURE1)
            return FEATURE_MACRO_FEATURE1;
        else if (macro == FEATURE_MACRO_STRING_BASE_WORD)
            return FEATURE_MACRO_BASE_WORD;
    }
    // bigram
    else {
        // bigram: left
        if (macro == FEATURE_MACRO_STRING_LEFT_WORD)
            return FEATURE_MACRO_LEFT_WORD;
        else if (macro == FEATURE_MACRO_STRING_LEFT_POS)
            return FEATURE_MACRO_LEFT_POS;
		else if (macro == FEATURE_MACRO_STRING_LEFT_SPOS)
			return FEATURE_MACRO_LEFT_SPOS;
		else if (macro == FEATURE_MACRO_STRING_LEFT_FORM)
			return FEATURE_MACRO_LEFT_FORM;
		else if (macro == FEATURE_MACRO_STRING_LEFT_FORM_TYPE)
			return FEATURE_MACRO_LEFT_FORM_TYPE;
        else if(macro == FEATURE_MACRO_STRING_LEFT_FUNCTIONAL_WORD)
            return FEATURE_MACRO_LEFT_FUNCTIONAL_WORD;
        else if (macro == FEATURE_MACRO_STRING_LEFT_LENGTH)
            return FEATURE_MACRO_LEFT_LENGTH;
        else if (macro == FEATURE_MACRO_STRING_LEFT_BEGINNING_CHAR)
            return FEATURE_MACRO_LEFT_BEGINNING_CHAR;
        else if (macro == FEATURE_MACRO_STRING_LEFT_ENDING_CHAR)
            return FEATURE_MACRO_LEFT_ENDING_CHAR;
        else if (macro == FEATURE_MACRO_STRING_LEFT_BEGINNING_CHAR_TYPE)
            return FEATURE_MACRO_LEFT_BEGINNING_CHAR_TYPE;
        else if (macro == FEATURE_MACRO_STRING_LEFT_ENDING_CHAR_TYPE)
            return FEATURE_MACRO_LEFT_ENDING_CHAR_TYPE;
        else if(macro == FEATURE_MACRO_STRING_LEFT_BASE_WORD)
            return FEATURE_MACRO_LEFT_BASE_WORD;
        // bigram: right
        else if (macro == FEATURE_MACRO_STRING_RIGHT_WORD)
            return FEATURE_MACRO_RIGHT_WORD;
        else if (macro == FEATURE_MACRO_STRING_RIGHT_POS)
            return FEATURE_MACRO_RIGHT_POS;
		else if (macro == FEATURE_MACRO_STRING_RIGHT_SPOS)
			return FEATURE_MACRO_RIGHT_SPOS;
		else if (macro == FEATURE_MACRO_STRING_RIGHT_FORM)
			return FEATURE_MACRO_RIGHT_FORM;
		else if (macro == FEATURE_MACRO_STRING_RIGHT_FORM_TYPE)
			return FEATURE_MACRO_RIGHT_FORM_TYPE;
        else if(macro == FEATURE_MACRO_STRING_RIGHT_FUNCTIONAL_WORD)
            return FEATURE_MACRO_RIGHT_FUNCTIONAL_WORD;
        else if (macro == FEATURE_MACRO_STRING_RIGHT_LENGTH)
            return FEATURE_MACRO_RIGHT_LENGTH;
        else if (macro == FEATURE_MACRO_STRING_RIGHT_BEGINNING_CHAR)
            return FEATURE_MACRO_RIGHT_BEGINNING_CHAR;
        else if (macro == FEATURE_MACRO_STRING_RIGHT_ENDING_CHAR)
            return FEATURE_MACRO_RIGHT_ENDING_CHAR;
        else if (macro == FEATURE_MACRO_STRING_RIGHT_BEGINNING_CHAR_TYPE)
            return FEATURE_MACRO_RIGHT_BEGINNING_CHAR_TYPE;
        else if (macro == FEATURE_MACRO_STRING_RIGHT_ENDING_CHAR_TYPE)
            return FEATURE_MACRO_RIGHT_ENDING_CHAR_TYPE;
        else if(macro == FEATURE_MACRO_STRING_RIGHT_BASE_WORD)
            return FEATURE_MACRO_RIGHT_BASE_WORD;
    }

    cerr << ";; cannot understand macro: " << macro << endl;
    return 0;
}//}}}

FeatureTemplate *FeatureTemplateSet::interpret_template(std::string &template_string, bool is_unigram) {//{{{
    std::vector<std::string> line;
    split_string(template_string, ":", line);
    return new FeatureTemplate(line[0], line[1], is_unigram);
}//}}}

bool FeatureTemplateSet::open(const std::string &template_filename) {//{{{
    std::ifstream ft_in(template_filename.c_str(), std::ios::in);
    if (!ft_in.is_open()) {
        cerr << ";; cannot open " << template_filename << " for reading" << endl;
        return false;
    }

    std::string buffer;
    while (getline(ft_in, buffer)) {
        if (buffer.at(0) == '#') // comment line
            continue;
        std::vector<std::string> line;
        split_string(buffer, " ", line);

        if (line[0] == "UNIGRAM") {
            templates.push_back(interpret_template(line[1], true));
        }
        else if (line[0] == "BIGRAM") {
            templates.push_back(interpret_template(line[1], false));
        }
        else {
            cerr << ";; cannot understand: " << line[0] << endl;
        }
    }

    return true;
}//}}}

std::string FeatureSet::str(){//{{{
    std::stringstream ss;
    for (auto it = fset.begin(); it != fset.end(); it++) {
        ss << *it << " x " << feature_weight[*it] << " ";
    }
    return ss.str();
};//}}}

bool FeatureSet::topic_available(Node* node){
    // 複合辞の処理
    // めぐる 除く のぞく 通じる 通ずる 通す 含める ふくめる 始める はじめる 絡む からむ 沿う そう 向ける 伴う ともなう 基づく よる 対する 関する 代わる おく つく とる 加える くわえる 限る 続く つづく 合わせる あわせる 比べる くらべる 並ぶ ならぶ おける いう する
    int not_hukugou_ji = 1;
    for( int j=0; j< NUM_OF_FUKUGOUJI;j++ ){
        not_hukugou_ji = not_hukugou_ji && (node->representation->compare(0, strlen(hukugouji[j]), hukugouji[j], strlen(hukugouji[j])));
    }
        
    // トピックを見る条件
    //(strstr(mrph->imis, "付属動詞候補" ) == NULL);
    int not_huzoku = (node->semantic_feature->find("付属動詞候補") == std::string::npos); //付属動詞でない
        
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
    int not_one_char = (node->char_num > 1); //表層が日本語1文字以上

    // トピックを読み込む条件
    int topical_cond = focused_hinsi && not_huzoku && not_suuryou && not_hukugou_ji && not_keisiki && not_timei_jinmei && not_jisou;
    return topical_cond;

    //auto result = db_get(topic_db, mrph->daihyo);

    // 文のトピックに入れるかどうかの条件
    //if(result && topical_cond && not_betu_yomi){ mrph->topical = 1; }
}

bool FeatureSet::topic_available_for_sentence(Node* node)
{
    // トピックを読み込む条件
    bool topical_cond = topic_available(node);
        
    // TODO:別の読みがないかどうかのチェック
    // node->bprev をたどっていけばよいはず．．．
    bool not_betu_yomi = true;
          
    // 読み込むのは後でよいのでは．．
    //auto result = db_get(topic_cdb, node->representation->c_str());
        
    // 文のトピックに入れるかどうかの条件 (現状, topic_available() と同じ)
    return (topical_cond && not_betu_yomi);
}

void read_vector(char* buf, std::vector<double> &vector) //topic用
{
    char *copy = strdup(buf);  
    char *copy_str = copy;
    char *token;
    char *saveptr1;
    int i;
    double drop;
    double sum=0.0;
        
    vector.resize(TOPIC_NUM);
    for (i = 0; i < TOPIC_NUM ; i++, copy=NULL){
        token = strtok_r(copy, " ", &saveptr1 );
        if (token == NULL)
            break;
        sscanf(token, "%lf", &(vector[i]));
//        // テンプレ for all_uniq_50
//        if (i == 1 || // テンプレート
//            i == 17|| // ヘッダ・フッダ
//            i == 27|| // ブログテンプレ
//            i == 35|| // テンプレ？
//            i == 46|| // テンプレ
//            i == 48|| // アフィリエイト
//            FALSE ){
//            vector[i] = 0.0;
//        }
        sum += vector[i]*vector[i];
    }
    sum = sqrt(sum);
    if(sum == 0.0){//これ役に立っている？
        //均等なベクトルをとりあえず指定?0ベクトル？
        //for (i = 0; i < TOPIC_NUM ; i++, copy=NULL){ vector[i] = 1.0; }
        sum = TOPIC_NUM;
    }
    // 正規化
    for (i = 0; i < TOPIC_NUM ; i++, copy=NULL){
        vector[i] = vector[i] / sum;
    }
    free(copy_str);
}


}

#include "common.h"
#include "pos.h"
#include "sentence.h"
#include "feature.h"
#include "scw.h"

// FeatureSet クラス
// 素性のセットを定義し，素性ベクトルへの変換を主に行う
// 現状，素性ベクトルを保持するのもこのクラス

// TODO: featureSet クラスから feature_weight を取り除く．tagger に移す．

namespace Morph {

std::vector<double>* FeatureSet::topic = nullptr;
bool FeatureSet::use_total_sim = false;
bool FeatureSet::debug_flag = false;
std::unordered_set<std::tuple<std::string, std::string, std::string, std::string>, tuple_hash, tuple_equal> FeatureSet::freq_word_set;

std::unordered_map<long int,std::string> FeatureSet::feature_map;
//std::tr1::unordered_map<std::vector<long int>,unsigned long> FeatureSet::feature_id_map;

FeatureSet::FeatureSet(FeatureTemplateSet *in_ftmpl) {//{{{
	ftmpl = in_ftmpl;
    weight = in_ftmpl->set_weight;
}//}}}

FeatureSet::~FeatureSet() {//{{{
	fset.clear(); //ここでdouble free? 
}//}}}

// TODO:デバッグ用の素性名生成を分離する．
void FeatureSet::extract_unigram_feature(Node *node) {//{{{
    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
    std::stringstream feature_name;
    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
        if (!((*tmpl_it)->get_is_unigram())) // skip bigram and trigram feature template
            continue;
        fv.clear();
        feature_name.str("");
        if(debug_flag) feature_name << (*tmpl_it)->get_name(); 
        fv.push_back((*tmpl_it)->get_name_hash()); 
        for (std::vector<unsigned int>::iterator it = (*tmpl_it)->get_features()->begin(); it != (*tmpl_it)->get_features()->end(); it++){
            if(debug_flag) feature_name<<":";
            if (*it == FEATURE_MACRO_WORD){ 
                if(debug_flag) feature_name << *(node->string);
                fv.push_back(get_feature_id(*(node->string)));
            }else if (*it == FEATURE_MACRO_POS){ //品詞
                if(debug_flag) feature_name << *(node->pos);
                fv.push_back(node->posid);
            }else if (*it == FEATURE_MACRO_SPOS){ //品詞細分類
                if(debug_flag) feature_name << *(node->spos);
                fv.push_back(node->sposid);
            }else if (*it == FEATURE_MACRO_FORM){ //活用形
                if(debug_flag) feature_name << *(node->form);
                fv.push_back(node->formid);
            }else if (*it == FEATURE_MACRO_FORM_TYPE){ //活用型
                if(debug_flag) feature_name << *(node->form_type);
                fv.push_back(node->formtypeid);
            }else if (*it == FEATURE_MACRO_FUNCTIONAL_WORD) { //機能語
                if( *(node->pos) == "助詞" || *(node->pos) == "助動詞" || *(node->pos) == "判定詞"){
                    if(debug_flag) feature_name << *(node->string) << "," << *(node->pos) << "," << *(node->spos);
                    fv.push_back(get_feature_id(*(node->string)));
                    fv.push_back(node->posid);
                    fv.push_back(node->sposid);
                    fv.push_back(0);
                }else{
                    if(debug_flag) feature_name << *(node->pos);
                    fv.push_back(node->posid);
                    fv.push_back(0);
                }
            } else if (*it == FEATURE_MACRO_BASE_WORD){ //原型
                if(debug_flag) feature_name << *(node->base);
                fv.push_back(node->baseid);
            } else if (*it == FEATURE_MACRO_DEVOICE){ //濁音化
                if ( node->stat == MORPH_DEVOICE_NODE){ //濁音化している
                    if(debug_flag) feature_name << "濁音化";
                    fv.push_back(1);
                }else{
                    if(debug_flag) feature_name << "-";
                    fv.push_back(2);
                }
            } else if (*it == FEATURE_MACRO_LENGTH){
                if(debug_flag) feature_name  << node->get_char_num();
                fv.push_back(node->get_char_num());
            } else if (*it == FEATURE_MACRO_LONGER){ // 辞書に登録されているよりも長い動的生成語彙(未知語, 数詞等)
                if(debug_flag && node->longer) feature_name << "+"; else feature_name << "-";
                fv.push_back(node->longer);
            } else if (*it == FEATURE_MACRO_NUMSTR){ // 数字としても解釈可能な単語につく素性
                if(debug_flag && node->suuji) feature_name << "+"; else feature_name << "-";
                fv.push_back(node->suuji);
            } else if (*it == FEATURE_MACRO_BEGINNING_CHAR){
                std::string f;
                f.append(node->get_first_char(), (node->stat & MORPH_PSEUDO_NODE) ? strlen(node->get_first_char()) : utf8_bytes((unsigned char *)node->get_first_char()));
                if(debug_flag) feature_name << f;
                fv.push_back(get_feature_id(f)); // 暫定
            } else if (*it == FEATURE_MACRO_ENDING_CHAR) {
                if(debug_flag) feature_name << *(node->end_string);
                fv.push_back(get_feature_id(*(node->end_string)));
            } else if (*it == FEATURE_MACRO_BEGINNING_CHAR_TYPE) {
                if(debug_flag) feature_name  << node->char_family; 
                fv.push_back(node->char_family);
            } else if (*it == FEATURE_MACRO_ENDING_CHAR_TYPE) {
                if(debug_flag) feature_name << node->end_char_family;
                fv.push_back(node->end_char_family);
            } else if (*it == FEATURE_MACRO_FEATURE1) { // Wikipedia (test)
                fv.push_back(node->lcAttr);
            } else if (*it == FEATURE_MACRO_NOMINALIZE) {
                if(node->semantic_feature && node->semantic_feature->find("名詞化",0) != std::string::npos){
                    if(debug_flag) feature_name << "名詞化";
                    fv.push_back(1);
                }else{
                    if(debug_flag) feature_name << "-";
                    fv.push_back(0);
                }
            } else if (*it == FEATURE_MACRO_LEXICAL) { // 
                if( freq_word_set.count(std::make_tuple(*(node->base),*(node->pos),*(node->spos),*(node->form_type))) > 0){
                    if(debug_flag) feature_name << *(node->string) << "," << *(node->pos) << "," << *(node->spos) << "," << *(node->form_type) << "," << *(node->form);
                    fv.push_back(get_feature_id(*(node->string)));
                    fv.push_back(node->posid);
                    fv.push_back(node->sposid);
                    fv.push_back(node->formtypeid);
                    fv.push_back(node->formid);
                    fv.push_back(0);
                }else{
                    if(debug_flag) feature_name << *(node->pos);
                    fv.push_back(node->posid);
                    fv.push_back(0);
                }
            }
        }
        auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
        fvec[feature_key]+=1;
        auto fmap = &feature_map[feature_key];
        if(fmap->empty()) *fmap = feature_name.str();
    }
}//}}}

// TODO: せめて，左右をまとめて扱えるようにする
void FeatureSet::extract_bigram_feature(Node *l_node, Node *r_node) {//{{{
    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
    std::stringstream feature_name;
    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
        if (!(*tmpl_it)->get_is_bigram()) // skip unigram and trigram feature template
            continue;
        fv.clear();
        feature_name.str("");
        if(debug_flag) feature_name << (*tmpl_it)->get_name(); 
        fv.push_back((*tmpl_it)->get_name_hash()); 
        for (std::vector<unsigned int>::iterator it = (*tmpl_it)->get_features()->begin(); it != (*tmpl_it)->get_features()->end(); it++) {
            if(debug_flag) feature_name<<":";
            if (*it == FEATURE_MACRO_LEFT_WORD){
                if(debug_flag) feature_name << *(l_node->string);
                fv.push_back(get_feature_id(*(l_node->string)));
            } else if (*it == FEATURE_MACRO_LEFT_POS){
                if(debug_flag) feature_name << *(l_node->pos);
                fv.push_back(l_node->posid);
            }else if (*it == FEATURE_MACRO_LEFT_SPOS){
                if(debug_flag) feature_name << *(l_node->spos);
                fv.push_back(l_node->sposid);
            }else if (*it == FEATURE_MACRO_LEFT_FORM){
                if(debug_flag) feature_name << *(l_node->form);
                fv.push_back(l_node->formid);
            }else if (*it == FEATURE_MACRO_LEFT_FORM_TYPE){
                if(debug_flag) feature_name << *(l_node->form_type);
                fv.push_back(l_node->formtypeid);
            }else if (*it == FEATURE_MACRO_LEFT_FUNCTIONAL_WORD){
                if( *(l_node->pos) == "助詞" || *(l_node->pos) == "助動詞" || *(l_node->pos) == "判定詞"){
                    if(debug_flag) feature_name << *(l_node->string) << "," << *(l_node->pos) << "," << *(l_node->spos);
                    fv.push_back(get_feature_id(*(l_node->string)));
                    fv.push_back(l_node->posid);
                    fv.push_back(l_node->sposid);
                    fv.push_back(0);
                }else{
                    if(debug_flag) feature_name << *(l_node->pos);
                    fv.push_back(l_node->posid);
                    fv.push_back(0);
                }
            } else if (*it == FEATURE_MACRO_LEFT_BASE_WORD){ //原型
                if(debug_flag) feature_name << *(l_node->base);
                fv.push_back(l_node->baseid);
            } else if (*it == FEATURE_MACRO_LEFT_LENGTH){
                if(debug_flag) feature_name << l_node->get_char_num();
                fv.push_back(l_node->get_char_num());
            } else if (*it == FEATURE_MACRO_LEFT_LONGER){
                if(debug_flag && l_node->longer) feature_name << "+"; else feature_name << "-";
                fv.push_back(l_node->longer);
            }else if (*it == FEATURE_MACRO_LEFT_BEGINNING_CHAR){
                std::string f;
                f.append(l_node->get_first_char(), (l_node->stat & MORPH_PSEUDO_NODE) ? strlen(l_node->get_first_char()) : utf8_bytes((unsigned char *)l_node->get_first_char()));
                if(debug_flag) feature_name << f;
                fv.push_back(get_feature_id(f)); // 暫定
            }else if (*it == FEATURE_MACRO_LEFT_ENDING_CHAR) {
                if(debug_flag) feature_name << *(l_node->end_string);
                fv.push_back(get_feature_id(*(l_node->end_string)));
            } else if (*it == FEATURE_MACRO_LEFT_BEGINNING_CHAR_TYPE) {
                if(debug_flag) feature_name << l_node->char_family; 
                fv.push_back(l_node->char_family);
            } else if (*it == FEATURE_MACRO_LEFT_ENDING_CHAR_TYPE) {
                if(debug_flag) feature_name << l_node->end_char_family; 
                fv.push_back(l_node->end_char_family);
            } else if (*it == FEATURE_MACRO_LEFT_DUMMY) { 
                fv.push_back(3);
            } else if (*it == FEATURE_MACRO_LEFT_LEXICAL){ // 
                if( freq_word_set.count(std::make_tuple(*(l_node->base),*(l_node->pos),*(l_node->spos),*(l_node->form_type))) > 0 ){
                    if(debug_flag) feature_name << *(l_node->string) << "," << *(l_node->pos) << "," << *(l_node->spos) << "," << *(l_node->form_type) << "," << *(l_node->form);
                    fv.push_back(get_feature_id(*(l_node->string)));
                    fv.push_back(l_node->posid);
                    fv.push_back(l_node->sposid);
                    fv.push_back(l_node->formtypeid);
                    fv.push_back(l_node->formid);
                    fv.push_back(0);
                } else {
                    if(debug_flag) feature_name << *(l_node->pos);
                    fv.push_back(l_node->posid);
                    fv.push_back(0);
                }
            }
            // right
            else if (*it == FEATURE_MACRO_RIGHT_WORD){
                if(debug_flag) feature_name << *(r_node->string);
                fv.push_back(get_feature_id(*(r_node->string)));
            } else if (*it == FEATURE_MACRO_RIGHT_POS){
                if(debug_flag) feature_name << *(r_node->pos);
                fv.push_back(r_node->posid);
            } else if (*it == FEATURE_MACRO_RIGHT_SPOS){
                if(debug_flag) feature_name << *(r_node->spos);
                fv.push_back(r_node->sposid);
            } else if (*it == FEATURE_MACRO_RIGHT_FORM){
                if(debug_flag) feature_name << *(r_node->form);
                fv.push_back(r_node->formid);
            }else if (*it == FEATURE_MACRO_RIGHT_FORM_TYPE){
                if(debug_flag) feature_name << *(r_node->form_type);
                fv.push_back(r_node->formtypeid);
            }else if (*it == FEATURE_MACRO_RIGHT_FUNCTIONAL_WORD){
                if( *(r_node->pos) == "助詞" || *(r_node->pos) == "助動詞" || *(r_node->pos) == "判定詞"){
                    if(debug_flag) feature_name << *(r_node->string) << "," << *(r_node->pos) << "," << *(r_node->spos);
                    fv.push_back(get_feature_id(*(r_node->string)));
                    fv.push_back(r_node->posid);
                    fv.push_back(r_node->sposid);
                    fv.push_back(0);
                }else{
                    if(debug_flag) feature_name << *(r_node->pos);
                    fv.push_back(r_node->posid);
                    fv.push_back(0);
                }
            }else if (*it == FEATURE_MACRO_RIGHT_BASE_WORD){ //原型
                if(debug_flag) feature_name << *(r_node->base);
                fv.push_back(r_node->baseid);
            }else if (*it == FEATURE_MACRO_RIGHT_LENGTH){
                if(debug_flag) feature_name << r_node->get_char_num();
                fv.push_back(r_node->get_char_num());
            }else if (*it == FEATURE_MACRO_RIGHT_LONGER){
                if(debug_flag && r_node->longer) feature_name << "+"; else feature_name << "-";
                fv.push_back(r_node->longer);
            }else if (*it == FEATURE_MACRO_RIGHT_BEGINNING_CHAR){
                std::string f;
                f.append(r_node->get_first_char(), (r_node->stat & MORPH_PSEUDO_NODE) ? strlen(r_node->get_first_char()) : utf8_bytes((unsigned char *)r_node->get_first_char()));
                if(debug_flag) feature_name << f;
                fv.push_back(get_feature_id(f));
            }else if (*it == FEATURE_MACRO_RIGHT_ENDING_CHAR){
                if(debug_flag) feature_name << *(r_node->end_string);
                fv.push_back(get_feature_id(*(r_node->end_string)));
            }else if (*it == FEATURE_MACRO_RIGHT_BEGINNING_CHAR_TYPE){
                if(debug_flag) feature_name << r_node->char_family; 
                fv.push_back(r_node->char_family);
            }else if (*it == FEATURE_MACRO_RIGHT_ENDING_CHAR_TYPE){
                if(debug_flag) feature_name << r_node->end_char_family; 
                fv.push_back(r_node->end_char_family);
            } else if (*it == FEATURE_MACRO_RIGHT_DUMMY){ 
                fv.push_back(3);
            } else if (*it == FEATURE_MACRO_RIGHT_LEXICAL){ // 
                if( freq_word_set.count(std::make_tuple(*(r_node->base),*(r_node->pos),*(r_node->spos),*(r_node->form_type))) > 0 ){
                    if(debug_flag) feature_name << *(r_node->string) << "," << *(r_node->pos) << "," << *(r_node->spos) << "," << *(r_node->form_type) << "," << *(r_node->form);
                    fv.push_back(get_feature_id(*(r_node->string)));
                    fv.push_back(r_node->posid);
                    fv.push_back(r_node->sposid);
                    fv.push_back(r_node->formtypeid);
                    fv.push_back(r_node->formid);
                    fv.push_back(0);
                } else {
                    if(debug_flag) feature_name << *(r_node->pos);
                    fv.push_back(r_node->posid);
                    fv.push_back(0);
                }
            }
        }
        auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
        fvec[feature_key]+=1;
        auto fmap = &feature_map[feature_key];
        if(fmap->empty()) *fmap = feature_name.str();
    }
}//}}}

void FeatureSet::extract_trigram_feature(Node *l_node, Node *m_node,  Node *r_node) {//{{{
    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
        if (! (*tmpl_it)->get_is_trigram()) // skip unigram and bigram feature template
            continue;
        fv.clear();
        fv.push_back((*tmpl_it)->get_name_hash()); 
        for (std::vector<unsigned int>::iterator it = (*tmpl_it)->get_features()->begin(); it != (*tmpl_it)->get_features()->end(); it++) {
            // left
            if (*it == FEATURE_MACRO_LEFT_WORD){
                fv.push_back(get_feature_id(*(l_node->string)));
            }else if (*it == FEATURE_MACRO_LEFT_POS){
                fv.push_back(l_node->posid);
            }else if (*it == FEATURE_MACRO_LEFT_SPOS){
                fv.push_back(l_node->sposid);
            }else if (*it == FEATURE_MACRO_LEFT_FORM){
                fv.push_back(l_node->formid);
            }else if (*it == FEATURE_MACRO_LEFT_FORM_TYPE){
                fv.push_back(l_node->formtypeid);
            }else if (*it == FEATURE_MACRO_LEFT_FUNCTIONAL_WORD){
                if( *(l_node->pos) == "助詞" || *(l_node->pos) == "助動詞" || *(l_node->pos) == "判定詞"){
                    fv.push_back(get_feature_id(*(l_node->string)));
                    fv.push_back(l_node->posid);
                    fv.push_back(l_node->sposid);
                    fv.push_back(0);
                }else{
                    fv.push_back(l_node->posid);
                    fv.push_back(0);
                }
            }
            else if (*it == FEATURE_MACRO_LEFT_BASE_WORD){ //原型
                fv.push_back(l_node->baseid);
            }else if (*it == FEATURE_MACRO_LEFT_LENGTH){
                fv.push_back(l_node->get_char_num());
            }else if (*it == FEATURE_MACRO_LEFT_LONGER){
                fv.push_back(l_node->longer);
            }else if (*it == FEATURE_MACRO_LEFT_BEGINNING_CHAR){
                std::string ftmp;
                ftmp.append(l_node->get_first_char(), (l_node->stat & MORPH_PSEUDO_NODE) ? strlen(l_node->get_first_char()) : utf8_bytes((unsigned char *)l_node->get_first_char()));
                fv.push_back(get_feature_id(ftmp)); // 暫定
            }else if (*it == FEATURE_MACRO_LEFT_ENDING_CHAR) {
                fv.push_back(get_feature_id(*(l_node->end_string)));
            } else if (*it == FEATURE_MACRO_LEFT_BEGINNING_CHAR_TYPE){
                fv.push_back(l_node->char_family);
            }else if (*it == FEATURE_MACRO_LEFT_ENDING_CHAR_TYPE){
                fv.push_back(l_node->end_char_family);
            } else if (*it == FEATURE_MACRO_LEFT_DUMMY){ 
                fv.push_back(3);
            } else if (*it == FEATURE_MACRO_LEFT_LEXICAL){ // 
                if( freq_word_set.count(std::make_tuple(*(l_node->base),*(l_node->pos),*(l_node->spos),*(l_node->form_type))) > 0 ){
                    fv.push_back(get_feature_id(*(l_node->string)));
                    fv.push_back(l_node->posid);
                    fv.push_back(l_node->sposid);
                    fv.push_back(l_node->formtypeid);
                    fv.push_back(l_node->formid);
                    fv.push_back(0);
                }else{
                    fv.push_back(l_node->posid);
                    fv.push_back(0);
                }
            }

            // right
            else if (*it == FEATURE_MACRO_RIGHT_WORD){
                fv.push_back(get_feature_id(*(r_node->string)));
            }else if (*it == FEATURE_MACRO_RIGHT_POS){
                fv.push_back(r_node->posid);
            }else if (*it == FEATURE_MACRO_RIGHT_SPOS){
                fv.push_back(r_node->sposid);
            }else if (*it == FEATURE_MACRO_RIGHT_FORM){
                fv.push_back(r_node->formid);
            }else if (*it == FEATURE_MACRO_RIGHT_FORM_TYPE){
                fv.push_back(r_node->formtypeid);
            }else if (*it == FEATURE_MACRO_RIGHT_FUNCTIONAL_WORD){
                if( *(r_node->pos) == "助詞" || *(r_node->pos) == "助動詞" || *(r_node->pos) == "判定詞"){
                    fv.push_back(get_feature_id(*(r_node->string)));
                    fv.push_back(r_node->posid);
                    fv.push_back(r_node->sposid);
                    fv.push_back(0);
                }else{
                    fv.push_back(r_node->posid);
                    fv.push_back(0);
                }
            }else if (*it == FEATURE_MACRO_RIGHT_BASE_WORD) {//原型
                fv.push_back(r_node->baseid);
            }else if (*it == FEATURE_MACRO_RIGHT_LENGTH){
                fv.push_back(r_node->get_char_num());
            }else if (*it == FEATURE_MACRO_RIGHT_LONGER){
                fv.push_back(r_node->longer);
            }else if (*it == FEATURE_MACRO_RIGHT_BEGINNING_CHAR){
                std::string ftmp;
                ftmp.append(r_node->get_first_char(), (r_node->stat & MORPH_PSEUDO_NODE) ? strlen(r_node->get_first_char()) : utf8_bytes((unsigned char *)r_node->get_first_char()));
                fv.push_back(get_feature_id(ftmp));
            }else if (*it == FEATURE_MACRO_RIGHT_ENDING_CHAR){
                fv.push_back(get_feature_id(*(r_node->end_string)));
            }else if (*it == FEATURE_MACRO_RIGHT_BEGINNING_CHAR_TYPE){
                fv.push_back(r_node->char_family);
            }else if (*it == FEATURE_MACRO_RIGHT_ENDING_CHAR_TYPE){
                fv.push_back(r_node->end_char_family);
            } else if (*it == FEATURE_MACRO_RIGHT_DUMMY) { 
                fv.push_back(3);
            } else if (*it == FEATURE_MACRO_RIGHT_LEXICAL) { // 
                if( freq_word_set.count(std::make_tuple(*(r_node->base),*(r_node->pos),*(r_node->spos),*(r_node->form_type))) > 0 ){
                    fv.push_back(get_feature_id(*(r_node->string)));
                    fv.push_back(r_node->posid);
                    fv.push_back(r_node->sposid);
                    fv.push_back(r_node->formtypeid);
                    fv.push_back(r_node->formid);
                    fv.push_back(0);
                }else{
                    fv.push_back(r_node->posid);
                    fv.push_back(0);
                }
            } else if (*it == FEATURE_MACRO_WORD) {
                fv.push_back(get_feature_id(*(m_node->string)));
            } else if (*it == FEATURE_MACRO_POS) { 
                fv.push_back(m_node->posid);
            } else if (*it == FEATURE_MACRO_SPOS) { //品詞細分類
                fv.push_back(m_node->sposid);
            } else if (*it == FEATURE_MACRO_FORM) { //活用形
                fv.push_back(m_node->formid);
            } else if (*it == FEATURE_MACRO_FORM_TYPE) { //活用型
                fv.push_back(m_node->formtypeid);
            } else if (*it == FEATURE_MACRO_FUNCTIONAL_WORD) { //機能語
                if( *(m_node->pos) == "助詞" || *(m_node->pos) == "助動詞" || *(m_node->pos) == "判定詞"){
                    fv.push_back(get_feature_id(*(m_node->string)));
                    fv.push_back(m_node->posid);
                    fv.push_back(m_node->sposid);
                    fv.push_back(0);
                }else{
                    fv.push_back(m_node->posid);
                    fv.push_back(0);
                }
            } else if (*it == FEATURE_MACRO_BASE_WORD) {//原型
                fv.push_back(m_node->baseid);
            } else if (*it == FEATURE_MACRO_DEVOICE) { //濁音化
                if ( m_node->stat == MORPH_DEVOICE_NODE) { //濁音化している
                    fv.push_back(1);
                } else {
                    fv.push_back(2);
                }
            } else if (*it == FEATURE_MACRO_LENGTH) { 
                fv.push_back(m_node->get_char_num());
            } else if (*it == FEATURE_MACRO_LONGER) { // 辞書に登録されているよりも長い動的生成語彙(未知語, 数詞等)
                fv.push_back(m_node->longer);
            } else if (*it == FEATURE_MACRO_NUMSTR) { // 数字としても解釈可能な単語につく素性
                fv.push_back(m_node->suuji);
            } else if (*it == FEATURE_MACRO_BEGINNING_CHAR) {
                std::string ftmp;
                ftmp.append(m_node->get_first_char(), (m_node->stat & MORPH_PSEUDO_NODE) ? strlen(m_node->get_first_char()) : utf8_bytes((unsigned char *)m_node->get_first_char()));
                fv.push_back(get_feature_id(ftmp)); // 暫定
            } else if (*it == FEATURE_MACRO_ENDING_CHAR) {
                fv.push_back(get_feature_id(*(m_node->end_string)));
            } else if (*it == FEATURE_MACRO_BEGINNING_CHAR_TYPE) {
                fv.push_back(m_node->char_family);
            } else if (*it == FEATURE_MACRO_ENDING_CHAR_TYPE) {
                fv.push_back(m_node->end_char_family);
            } else if (*it == FEATURE_MACRO_FEATURE1) {// Wikipedia (test)
                fv.push_back(m_node->lcAttr);
            } else if (*it == FEATURE_MACRO_LEXICAL) { // 
                if( freq_word_set.count(std::make_tuple(*(m_node->base),*(m_node->pos),*(m_node->spos),*(m_node->form_type))) > 0){
                    fv.push_back(get_feature_id(*(m_node->string)));
                    fv.push_back(m_node->posid);
                    fv.push_back(m_node->sposid);
                    fv.push_back(m_node->formtypeid);
                    fv.push_back(m_node->formid);
                    fv.push_back(0);
                }else{
                    fv.push_back(m_node->posid);
                    fv.push_back(0);
                }
            }
        }
        auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
        fvec[feature_key]+=1;
    }
}//}}}

void FeatureSet::extract_context_feature(double context_score) {//{{{
    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
    fv.clear();
    const unsigned long feature_name = get_feature_id("context_feature");
    fv.push_back(feature_name);

    if(context_score <=0){
        fv.push_back(get_feature_id("NaN"));
        return; 
    }

    double logalistic_score = std::log(context_score);
        
    int bin_index = static_cast<int>( std::round(logalistic_score * 10));
    if(bin_index > -10) bin_index = -10; //極端に高いスコアは珍しく，意味が薄い
    if(bin_index < -100) bin_index = -100; //極端に低いスコアも同様
    fv.push_back(bin_index/10);
        
    auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
    fvec[feature_key]+=1;
}//}}}

void FeatureSet::extract_topic_feature(Node *node) {//{{{
    if(FeatureSet::topic){ // TOPIC 素性 ( TODO: ハードコードをやめてルール化
        if(node->topic_available()){
            if(FeatureSet::use_total_sim){
                TopicVector node_topic = node->get_topic();
                double sum = 0.0;
                for(size_t i = 0; i< node_topic.size();i++){
                    sum += node_topic[i] * (*topic)[i];
                }
                fset.push_back("TP_all:" + binning(sum));
            }else{
                TopicVector node_topic = node->get_topic();
                for(size_t i = 0; i< node_topic.size();i++){
                    //std::cerr << "TP" << int2string(i) << ":" << binning(node_topic[i] * (*topic)[i]) << " " ;
                    //fset.push_back("TP" + int2string(i) + ":"  + binning(node_topic[i] * (*topic)[i]));
                    
                    fset.push_back("TP" + int2string(i) + ":"  + binning((*topic)[i] - node_topic[i])); //差
                }
            }
        }else{
            fset.push_back(std::string("TOPIC:<NONE>"));
        }
    }
}//}}}

bool FeatureSet::append_feature(FeatureSet *in) {//{{{
    for (std::vector<std::string>::iterator it = in->fset.begin(); it != in->fset.end(); it++) {
        fset.push_back(*it);
        //fvec[*it]=1.0;
    }
    return true;
}//}}}

// 廃止を検討中
double FeatureSet::calc_inner_product_with_weight() {//{{{
    if(!weight) return 0;
    return (*weight)*fvec; 

}//}}}

bool FeatureSet::print() {//{{{
    for (std::vector<std::string>::iterator it = fset.begin(); it != fset.end(); it++) {
        cerr << *it << " ";
    }
    cerr << endl;
    return true;
}//}}}

FeatureTemplate::FeatureTemplate(std::string &in_name, std::string &feature_string, int in_n_gram) {//{{{
    is_unigram = (in_n_gram == 1);
    is_bigram = (in_n_gram == 2);
    is_trigram = (in_n_gram == 3);
    name = in_name;
    std::hash<std::string> shash;
    feature_name_hash = shash(in_name);
        
    std::vector<std::string> line;
    split_string(feature_string, ",", line);
    
    // 素性列
    for (std::vector<std::string>::iterator it = line.begin(); it != line.end(); it++) {

//        // 旧版
        unsigned int macro_id = interpret_macro(*it);
        if (macro_id){
            features.push_back(macro_id);
        }

        // 新板 (保留)
//        auto macro = interpret_macro_function(*it);
//        feature_functions.push_back(macro);
    }
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
            templates.push_back(interpret_template(line[1], 1));
        }
        else if (line[0] == "BIGRAM") {
            templates.push_back(interpret_template(line[1], 2));
        }
        else if (line[0] == "TRIGRAM") {
            templates.push_back(interpret_template(line[1], 3));
        }
        else {
            cerr << ";; cannot understand: " << line[0] << endl;
        }
    }

    return true;
}//}}}

bool FeatureSet::open_freq_word_set(const std::string &list_filename) {//{{{
    std::ifstream ft_in(list_filename.c_str(), std::ios::in);
    if (!ft_in.is_open()) {
        cerr << ";; cannot open " << list_filename << " for reading" << endl;
        return false;
    }
        
    std::string buffer;
    while (getline(ft_in, buffer)) {
        if (buffer.at(0) == '#') // comment line
            continue;
        std::vector<std::string> line;
        split_string(buffer, " ", line);
            
        //std::cerr << "f: " << line[0] << "_" << line[1] << "_" << line[2] << "_" << line[3] << std::endl;
        freq_word_set.insert(std::make_tuple(line[0],line[1],line[2],line[3]));
        //freq_word_set.insert(line[0] + "_" + line[1] + "_" + line[2] + "_" + line[3]);
    }
    return true;
}//}}}

std::string FeatureSet::str(){//{{{
    std::stringstream ss;
    std::vector<std::pair<std::string,double>> fpair;
    for (auto it = fvec.begin(); it != fvec.end(); it++) {
        ss.str("");
        ss << feature_map[it->first] << "x" << it->second * (*weight)[it->first];
        fpair.push_back(make_pair(ss.str(), it->second * (*weight)[it->first]));
    }

    // 素性を重み順にソート
    // std::sort(fpair.begin(), fpair.end(),[](std::pair<std::string,double> &left,std::pair<std::string,double> &right)->int{return (std::abs(left.second) > std::abs(right.second));});
    
    ss.str("");
    for (auto it = fpair.begin(); it != fpair.end(); it++) {
        if(std::abs(it->second) > 0.5 )
            ss << "\e[31m" << it->first << "\e[39m" << " "; //Red
        else if(std::abs(it->second) > 0.1 )
            ss << "\e[35m" << it->first << "\e[39m" << " "; //Yellow
        else if(std::abs(it->second) > 0.05 )
            ss << "\e[33m" << it->first << "\e[39m" << " "; //Yellow
        else
            ss << it->first << " ";
            
    }

    return ss.str();
};//}}}

FeatureTemplate *FeatureTemplateSet::interpret_template(std::string &template_string, int n_gram) {//{{{
    std::vector<std::string> line;
    split_string(template_string, ":", line);
    return new FeatureTemplate(line[0], line[1], n_gram);
}//}}}

unsigned int FeatureTemplate::interpret_macro(std::string &macro) {//{{{

        // ここでIDではなく文字列のまま持っておく
//        NodePosition pos; //{LeftN,RightN,CenterN,UnigramN};
//        if (*it.find("%R") != std::string::npos) {
//            pos = RightN; // R
//        } else if (*it.find("%L") != std::string::npos) {
//            pos = LeftN; // L
//        } else if (*it.find("%C") != std::string::npos) {
//            pos = CenterN; // C
//        } else {
//            pos = UnigramN; // U
//        }

    // unigram, trigram
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
    else if (macro == FEATURE_MACRO_STRING_DEVOICE)
        return FEATURE_MACRO_DEVOICE;
    else if (macro == FEATURE_MACRO_STRING_LONGER)
        return FEATURE_MACRO_LONGER;
    else if (macro == FEATURE_MACRO_STRING_NUMSTR)
        return FEATURE_MACRO_NUMSTR;
    else if (macro == FEATURE_MACRO_STRING_LEXICAL)
        return FEATURE_MACRO_LEXICAL;
    else if (macro == FEATURE_MACRO_STRING_NOMINALIZE)
        return FEATURE_MACRO_NOMINALIZE;

    // bigram: left
    else if (macro == FEATURE_MACRO_STRING_LEFT_WORD)
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
    else if(macro == FEATURE_MACRO_STRING_LEFT_PREFIX)
        return FEATURE_MACRO_LEFT_PREFIX;
    else if(macro == FEATURE_MACRO_STRING_LEFT_SUFFIX)
        return FEATURE_MACRO_LEFT_SUFFIX;
    else if(macro == FEATURE_MACRO_STRING_LEFT_DUMMY)
        return FEATURE_MACRO_LEFT_DUMMY;
    else if (macro == FEATURE_MACRO_STRING_LEFT_LONGER)
        return FEATURE_MACRO_LEFT_LONGER;
    else if (macro == FEATURE_MACRO_STRING_LEFT_NUMSTR)
        return FEATURE_MACRO_LEFT_NUMSTR;
    else if (macro == FEATURE_MACRO_STRING_LEFT_LEXICAL)
        return FEATURE_MACRO_LEFT_LEXICAL;

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
    else if(macro == FEATURE_MACRO_STRING_RIGHT_PREFIX)
        return FEATURE_MACRO_RIGHT_PREFIX;
    else if(macro == FEATURE_MACRO_STRING_RIGHT_SUFFIX)
        return FEATURE_MACRO_RIGHT_SUFFIX;
    else if(macro == FEATURE_MACRO_STRING_RIGHT_DUMMY)
        return FEATURE_MACRO_RIGHT_DUMMY;
    else if (macro == FEATURE_MACRO_STRING_RIGHT_LONGER)
        return FEATURE_MACRO_RIGHT_LONGER;
    else if (macro == FEATURE_MACRO_STRING_RIGHT_NUMSTR)
        return FEATURE_MACRO_RIGHT_NUMSTR;
    else if (macro == FEATURE_MACRO_STRING_RIGHT_LEXICAL)
        return FEATURE_MACRO_RIGHT_LEXICAL;

cerr << ";; cannot understand macro: " << macro << endl;
return 0;
}//}}}

// 以下は保留

//// 素性関数 速度面に問題があるためまとめて保留中, 可変長テンプレートで素性ごとに定義しておけばなんとか・・// /*{{{*/
//// TODO:あとで素性関数のクラスにまとめる
//std::hash<std::string> hash_func; 
//inline unsigned long get_feature_id(const std::string& s){ return hash_func(s);};
//
//// 素性が解釈出来なかった時用の何もしない素性生成関数
//void feature_function_noop() { }
//
////// 接頭辞素性 
////void feature_function_prefix(Node* l_node, Node* r_node, FeatureTemplate::FTemp& fv, NodePosition position){/*{{{*/
////    if(pos == LeftN){
////        if( *(l_node->pos) != "接頭辞"){
////            fv.push_back(1);
////        } else if((*(l_node->spos) == "名詞接頭辞" && *(r_node->pos) == "名詞") || 
////                (*(l_node->spos) == "動詞接頭辞" && *(r_node->pos) == "動詞") || 
////                (*(l_node->spos) == "イ形容詞接尾辞" && *(r_node->pos) == "形容詞" && 
////                 (*(r_node->form_type) == "イ形容詞アウオ段"|| 
////                  *(r_node->form_type) == "イ形容詞イ段"|| 
////                  *(r_node->form_type) == "イ形容詞イ段特殊")) || 
////                (*(l_node->spos) == "ナ形容詞接頭辞" && *(r_node->pos) == "形容詞" &&
////                 (*(r_node->form_type) == "ナ形容詞"|| 
////                  *(r_node->form_type) == "ナノ形容詞"|| 
////                  *(r_node->form_type) == "ナ形容詞特殊")) ){
////            fv.push_back(2);
////        } else {
////            fv.push_back(0);
////        }
////    }else{//dummy
////        fv.push_back(3);
////    }
////}/*}}}*/
////
////// 接尾辞素性 
////void feature_function_prefix(Node* m_node, Node* r_node,FeatureTemplate::FTemp& fv, NodePosition position){/*{{{*/
////    if(pos=RightN){
////        if(*(r_node->pos) != "接尾辞" ){
////            fv.push_back(1);
////        } else if(  (*(m_node->spos) == "数詞" && *(r_node->spos) == "名詞性名詞助数辞") || // ３枚
////                // 名詞相当 + 名詞接尾辞
////                (((*(m_node->pos) == "名詞" && *(m_node->spos) != "数詞") || 
////                  (*(m_node->pos) == "動詞" && *(m_node->spos) == "基本連用形") || 
////                  (*(m_node->pos) == "形容詞") ||
////                  (*(m_node->spos) == "名詞性名詞接尾辞")||
////                  (*(m_node->spos) == "名詞性名詞助数字")||
////                  (*(m_node->spos) == "名詞性特殊接尾辞")) && 
////                 ( *(r_node->spos) == "名詞性名詞接尾辞" || // 〜後 〜化
////                   *(r_node->spos) == "名詞性特殊接尾辞" || // 〜さん 〜移行
////                   *(r_node->spos) == "形容詞性名詞接尾辞")) || // 〜的 〜的だ
////                // 働か+ない 
////                ((*(m_node->pos) == "動詞"||*(m_node->pos) == "形容詞"||*(m_node->spos) == "動詞性接尾辞")&& 
////                 (*(r_node->spos) == "動詞性接尾辞" || // "させない"の"せ"
////                  *(r_node->spos) == "名詞性述語接尾辞" || // "高さ" の"さ" 聞き手の"手" 食べ放題の"放題"
////                  *(r_node->spos) == "形容詞性述語接尾辞"))) { //"させない" の"ない"
////            fv.push_back(2);
////        } else { 
////            fv.push_back(0);
////        }
////    }else{
////        fv.push_back(3)
////    }
////}/*}}}*/
//
//// ダミー素性(lambda版)
//auto feature_function_dummy = [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(3);
//};
//
//// 表層素性
//auto feature_function_word = [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(get_feature_id(*(node->string)));
//};
//
//// 長さ素性
//auto feature_function_length= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->get_char_num());
//};
//
//// 原形
//auto feature_function_base_word= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->baseid);
//};
//
//// 品詞素性
//auto feature_function_pos= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->posid);
//};
//
//// 細分類
//auto feature_function_spos= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->sposid);
//};
//
//// 活用形
//auto feature_function_form= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->formid);
//};
//
//// 機能語
//auto feature_function_functional_word= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    if( *(node->pos) == "助詞" || *(node->pos) == "助動詞" || *(node->pos) == "判定詞") {
//        fv.push_back(get_feature_id(*(node->string)));
//        fv.push_back(node->posid);
//        fv.push_back(node->sposid);
//        fv.push_back(0);
//    } else {
//        // 長さが変わる
//        fv.push_back(node->posid);
//        fv.push_back(0);
//    }
//};
//
//// 活用型
//auto feature_function_form_type= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->formtypeid);
//};
//
//// 最初の文字
//auto feature_function_beginning_char= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    std::string f;
//    f.append(node->get_first_char(), (node->stat & MORPH_PSEUDO_NODE) ? strlen(node->get_first_char()) : utf8_bytes((unsigned char *)node->get_first_char()));
//    fv.push_back(get_feature_id(f)); // 暫定
//};
//
//// 最初の文字タイプ
//auto feature_function_biginning_char_type= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->char_family);
//};
//
//// 最後の文字
//auto feature_function_ending_char= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(get_feature_id(*(node->end_string)));
//};
//
//// 最後の文字タイプ
//auto feature_function_ending_char_type= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->end_char_family);
//};
//
//// 濁音化
//auto feature_function_devoice= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    // 元はunigram, trigram のCのみだったが，他についても定義してしまっても問題ない
//    if ( node->stat == MORPH_DEVOICE_NODE){ //濁音化している
//        fv.push_back(1);
//    }else{
//        fv.push_back(2);
//    }
//};
//
//// 辞書長との比較素性
//auto feature_function_longer= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->longer);
//};
//
//// 数字素性
//auto feature_function_numstr= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->suuji);
//};
//
//// 語彙化素性
//auto feature_function_lexical= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    // freq_word_set
//    if( FeatureSet::freq_word_set.count(std::make_tuple(*(node->base),*(node->pos), *(node->spos),*(node->form_type))) > 0) {
//        fv.push_back(get_feature_id(*(node->string)));
//        fv.push_back(node->posid);
//        fv.push_back(node->sposid);
//        fv.push_back(node->formtypeid);
//        fv.push_back(node->formid);
//        fv.push_back(0);
//    } else {
//        fv.push_back(node->posid);
//        fv.push_back(0);
//    }
//};
//
//// テスト用の素性
//auto feature_function_feature1= [](Node* node, FeatureTemplate::FTemp& fv, NodePosition position) -> void {
//    fv.push_back(node->lcAttr);
//};
//
//// 素性名と素性関数の対応付け 
//std::unordered_map<std::string, std::function<void(Node*,FeatureTemplate::FTemp&,NodePosition)>> FeatureTemplate::feature_map_function = {
//{"w", feature_function_word},
//{"p", feature_function_pos},
//{"l", feature_function_length},
//{"sp", feature_function_spos},
//{"sf", feature_function_form},
//{"sft", feature_function_form_type},
//{"f", feature_function_functional_word},
//{"ba", feature_function_base_word},
//{"bc", feature_function_beginning_char},
//{"ec", feature_function_ending_char},
//{"bt", feature_function_biginning_char_type},
//{"et", feature_function_ending_char_type},
//{"devoice", feature_function_devoice},
//{"longer", feature_function_longer},
//{"numstr", feature_function_numstr},
//{"lexical", feature_function_lexical},
//{"f1", feature_function_feature1}, // テスト用素性名
////{"prefix", feature_function_prefix},
////{"suffix", feature_function_suffix },
//{"dummy", feature_function_dummy},
//};/*}}}*/

//FeatureTemplate::FeatureFunction FeatureTemplate::interpret_macro_function(std::string &macro) { //{{{
//
//    NodePosition pos; //{LeftN,RightN,CenterN,UnigramN};
//    std::string feature_name;
//    size_t fname_pos;
//    if ( (fname_pos = macro.find("%R")) != std::string::npos) {
//        pos = RightN; // R
//        feature_name = macro.substr(fname_pos+2);
//    } else if ( (fname_pos = macro.find("%L")) != std::string::npos) {
//        pos = LeftN; // L
//        feature_name = macro.substr(fname_pos+2);
//    } else if ( (fname_pos = macro.find("%C")) != std::string::npos) {
//        pos = CenterN; // C
//        feature_name = macro.substr(fname_pos+2);
//    } else if ( (fname_pos = macro.find("%")) != std::string::npos) { 
//        pos = UnigramN; // U
//        feature_name = macro.substr(fname_pos+1);
//    } else {
//        std::cerr << "Error@interpret_macro_function " << std::endl;
//        std::cerr << ";; cannot parse macro name: " << macro << endl;
//        pos = UnigramN; // U
//        feature_name = macro;
//    }
//
//    auto func_itr = feature_map_function.find(feature_name);
//    if(func_itr != feature_map_function.end() ){
//        if(pos == RightN)
//            return std::bind( func_itr->second, std::placeholders::_3, std::placeholders::_4, pos);
//        else if(pos == CenterN)
//            return std::bind( func_itr->second, std::placeholders::_2, std::placeholders::_4, pos);
//        else if(pos == LeftN)
//            return std::bind( func_itr->second, std::placeholders::_1, std::placeholders::_4, pos);
//        else //UnigramN
//            return std::bind( func_itr->second, std::placeholders::_2, std::placeholders::_4, pos);
//    }else{
//        std::cerr << "Error@interpret_macro_function " << std::endl;
//        std::cerr << ";; cannot understand feature_name: " << feature_name << " pos=" << pos << endl;
//        std::cerr << ";; cannot understand macro: " << macro  << endl;
//        return std::bind( feature_function_noop ); //何もしない
//    }
//}//}}}

//void FeatureSet::extract_unigram_feature(Node *node) {//{{{
//    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
//    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
//        // TODO: unigram, bi-gram, trigram でそもそも別のvector にしては．
//        if (!((*tmpl_it)->get_is_unigram())) // skip bigram and trigram feature template
//            continue;
//        fv.clear();
//
//        // 素性のタイプを表す ID
//        fv.push_back((*tmpl_it)->get_name_hash()); 
//         
//        // 呼び出しがinline化できないから遅い. 
//        for (auto it = (*tmpl_it)->get_feature_functions().begin(); it != (*tmpl_it)->get_feature_functions().end(); it++) {
//            auto feature_func = *it;
//            feature_func(nullptr, node, nullptr, fv);
//        }
//
//        auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
//        //std::cerr << feature_key << std::endl;
//        fvec[feature_key]+=1;
//    }
//}//}}}
//
//void FeatureSet::extract_bigram_feature(Node *l_node, Node *r_node) { //{{{
//    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
//    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
//        // TODO: unigram, bi-gram, trigram でそもそも別のvector にしては．
//        if (!((*tmpl_it)->get_is_bigram())) // skip bigram and trigram feature template
//            continue;
//        fv.clear();
//
//        // 素性のタイプを表す ID
//        // fv.push_back(std::distance(ftmpl->get_templates()->begin(),tmpl_it)); 
//        fv.push_back((*tmpl_it)->get_name_hash()); // TODO: モデルの再生成が必要になるため保留
//        
//        for (auto it = (*tmpl_it)->get_feature_functions().begin(); it != (*tmpl_it)->get_feature_functions().end(); it++) {
//            auto feature_func = *it;
//            feature_func(l_node,nullptr,r_node, fv);
//        }
//        auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
//        fvec[feature_key]+=1;
//    }
//}//}}}
//
//void FeatureSet::extract_trigram_feature(Node *l_node, Node *m_node, Node *r_node) {//{{{
//    static std::vector<unsigned long> fv; // TODO:外部から与える形に変更する.
//    for (std::vector<FeatureTemplate *>::iterator tmpl_it = ftmpl->get_templates()->begin(); tmpl_it != ftmpl->get_templates()->end(); tmpl_it++) {
//        if (!((*tmpl_it)->get_is_trigram())) // skip bigram and trigram feature template
//            continue;
//        fv.clear();
//
//        // 素性のタイプを表す ID
//        // fv.push_back(std::distance(ftmpl->get_templates()->begin(),tmpl_it)); 
//        fv.push_back((*tmpl_it)->get_name_hash()); // TODO: モデルの再生成が必要になるため保留
//        
//        for (auto it = (*tmpl_it)->get_feature_functions().begin(); it != (*tmpl_it)->get_feature_functions().end(); it++) {
//            auto feature_func = *it;
//            feature_func(l_node,m_node,r_node, fv);
//        }
//        auto feature_key = boost::hash_range(fv.begin(),fv.end()); 
//        fvec[feature_key]+=1;
//    }
//}//}}}

}


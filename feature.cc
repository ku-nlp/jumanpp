#include "common.h"
#include "pos.h"
#include "sentence.h"
#include "feature.h"
#include "scw.h"

// TODO: featureSet クラスから feature_weight を取り除く．tagger に移す．

namespace Morph {
std::vector<double>* FeatureSet::topic = nullptr;


FeatureSet::FeatureSet(FeatureTemplateSet *in_ftmpl) {//{{{
	ftmpl = in_ftmpl;
    weight = in_ftmpl->set_weight;
}//}}}

FeatureSet::~FeatureSet() {//{{{
	fset.clear();
}//}}}

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

    // Topic 関係の素性を追加
    extract_topic_feature(node);

}//}}}

void FeatureSet::extract_topic_feature(Node *node) {//{{{
    if(FeatureSet::topic){ // TOPIC 素性 ( TODO: ハードコードをやめてルール化
        if(node->topic_available()){
            TopicVector node_topic = node->get_topic();
            for(size_t i = 0; i< node_topic.size();i++){
                //std::cerr << "TP" << int2string(i) << ":" << binning(node_topic[i] * (*topic)[i]) << " " ;
                //fset.push_back("TP" + int2string(i) + ":"  + binning(node_topic[i] * (*topic)[i]));
                fset.push_back("TP" + int2string(i) + ":"  + binning((*topic)[i] - node_topic[i])); //差
            }
        }else{
            fset.push_back(std::string("TOPIC:<NONE>"));
        }
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
            else if (*it == FEATURE_MACRO_LEFT_PREFIX){ //接頭辞の種類と後続する品詞が一致しているか
                if( *(l_node->pos) != "接頭辞" )
                    f += "neg";
                else if((*(l_node->spos) == "名詞接頭辞" && *(r_node->pos) == "名詞") || 
                        (*(l_node->spos) == "動詞接頭辞" && *(r_node->pos) == "動詞") || 
                        (*(l_node->spos) == "イ形容詞接尾辞" && *(r_node->pos) == "形容詞" && 
                         (*(r_node->form_type) == "イ形容詞アウオ段"|| 
                          *(r_node->form_type) == "イ形容詞イ段"|| 
                          *(r_node->form_type) == "イ形容詞イ段特殊")) || 
                        (*(l_node->spos) == "ナ形容詞接頭辞" && *(r_node->pos) == "形容詞" &&
                         (*(r_node->form_type) == "ナ形容詞"|| 
                          *(r_node->form_type) == "ナノ形容詞"|| 
                          *(r_node->form_type) == "ナ形容詞特殊")) )
                    f += "1";
                else
                    f += "0";
            }
            else if (*it == FEATURE_MACRO_LEFT_SUFFIX){ 
                f += "dummy";
            }
            else if (*it == FEATURE_MACRO_LEFT_DUMMY){ 
                f += "dummy";
            }

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
            else if (*it == FEATURE_MACRO_RIGHT_SUFFIX){ //接尾辞が直前の品詞と一致しているか
                if(*(r_node->pos) != "接尾辞" )
                    f += "neg";
                else if( (*(l_node->spos) == "数詞" && *(r_node->spos) == "名詞性名詞助数辞") || 
                    ( ((*(l_node->pos) == "名詞" && *(l_node->spos) != "数詞") || 
                       (*(l_node->pos) == "動詞" && *(l_node->spos) == "基本連用形") || 
                       (*(l_node->pos) == "形容詞" ) ) && 
                        ( *(r_node->spos) == "名詞性名詞接尾辞" || 
                          *(r_node->spos) == "名詞性特殊接尾辞" || 
                          *(r_node->spos) == "形容詞性名詞接尾辞"))||
                    (*(l_node->pos) == "動詞" && 
                        (*(r_node->spos) == "動詞性接尾辞" || 
                         *(r_node->spos) == "名詞性述語接尾辞" ||
                         *(r_node->spos) == "形容詞性述語接尾辞")))
                    f += "1";
                else
                    f += "0";
            }
            else if (*it == FEATURE_MACRO_RIGHT_SUFFIX){ 
                f += "dummy";
            }
            else if (*it == FEATURE_MACRO_RIGHT_DUMMY){ 
                f += "dummy";
            }
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
    if(!weight) return 0;
    for (std::vector<std::string>::iterator it = fset.begin(); it != fset.end(); it++) {
        auto itr = weight->find(*it);
        if(itr != weight->end()){
            sum += itr->second;
        }
//        if(weight->has_key(*it))
//            sum += (*weight)[*it];
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
        else if(macro == FEATURE_MACRO_STRING_LEFT_PREFIX)
            return FEATURE_MACRO_LEFT_PREFIX;
        else if(macro == FEATURE_MACRO_STRING_LEFT_SUFFIX)
            return FEATURE_MACRO_LEFT_SUFFIX;
        else if(macro == FEATURE_MACRO_STRING_LEFT_DUMMY)
            return FEATURE_MACRO_LEFT_DUMMY;
        
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
        ss << *it << "x" << (*weight)[*it] << " ";
    }
    return ss.str();
};//}}}

}

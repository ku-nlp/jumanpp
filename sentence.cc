#include "common.h"
#include "pos.h"
#include "sentence.h"
#include "feature.h"

namespace Morph {

std::string BOS_STRING = BOS;
std::string EOS_STRING = EOS;

// for test sentence
Sentence::Sentence(std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, std::string &in_sentence, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param) {
    sentence_c_str = in_sentence.c_str();
    length = strlen(sentence_c_str);
    init(length, in_begin_node_list, in_end_node_list, in_dic, in_ftmpl, in_param);
}

// for gold sentence
Sentence::Sentence(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param) {
    length = 0;
    init(max_byte_length, in_begin_node_list, in_end_node_list, in_dic, in_ftmpl, in_param);
}

void Sentence::init(size_t max_byte_length, std::vector<Node *> *in_begin_node_list, std::vector<Node *> *in_end_node_list, Dic *in_dic, FeatureTemplateSet *in_ftmpl, Parameter *in_param) {
    param = in_param;
    dic = in_dic;
    ftmpl = in_ftmpl;
    word_num = 0;
    feature = NULL;
    reached_pos = 0;
    reached_pos_of_pseudo_nodes = 0;

    begin_node_list = in_begin_node_list;
    end_node_list = in_end_node_list;

    if (begin_node_list->capacity() <= max_byte_length) {
        begin_node_list->reserve(max_byte_length + 1);
        end_node_list->reserve(max_byte_length + 1);
    }
    memset(&((*begin_node_list)[0]), 0, sizeof((*begin_node_list)[0]) * (max_byte_length + 1));
    memset(&((*end_node_list)[0]), 0, sizeof((*end_node_list)[0]) * (max_byte_length + 1));

    (*end_node_list)[0] = get_bos_node(); // Begin Of Sentence
}

Sentence::~Sentence() {
    if (feature)
        delete feature;
    clear_nodes();
}

void Sentence::clear_nodes() {
    if ((*end_node_list)[0])
        delete (*end_node_list)[0]; // delete BOS
    for (unsigned int pos = 0; pos <= length; pos++) {
        Node *tmp_node = (*begin_node_list)[pos];
        while (tmp_node) {
            Node *next_node = tmp_node->bnext;
            delete tmp_node;
            tmp_node = next_node;
        }
    }
    memset(&((*begin_node_list)[0]), 0, sizeof((*begin_node_list)[0]) * (length + 1));
    memset(&((*end_node_list)[0]), 0, sizeof((*end_node_list)[0]) * (length + 1));
}

bool Sentence::add_one_word(std::string &word) {
    word_num++;
    length += strlen(word.c_str());
    sentence += word;
    return true;
}

void Sentence::feature_print() {
    feature->print();
}

// make unknown word candidates of specified length if it's not found in dic
Node *Sentence::make_unk_pseudo_node_list_by_dic_check(const char *start_str, unsigned int pos, Node *r_node, unsigned int specified_char_num) {
    bool find_this_length = false;
    Node *tmp_node = r_node;
    while (tmp_node) {
        if (tmp_node->char_num == specified_char_num) {
            find_this_length = true;
            break;
        }
        tmp_node = tmp_node->bnext;
    }

    if (!find_this_length) { // if a node of this length is not found in dic
        Node *result_node = dic->make_unk_pseudo_node_list(start_str + pos, specified_char_num, specified_char_num);
        if (result_node) {
            if (r_node) {
                tmp_node = result_node;
                while (tmp_node->bnext)
                    tmp_node = tmp_node->bnext;
                tmp_node->bnext = r_node;
            }
            return result_node;
        }
    }
    return r_node;
}

Node *Sentence::make_unk_pseudo_node_list_from_previous_position(const char *start_str, unsigned int previous_pos) {
    if ((*end_node_list)[previous_pos] != NULL) {
        Node **node_p = &((*begin_node_list)[previous_pos]);
        while (*node_p) {
            node_p = &((*node_p)->bnext);
        }
        *node_p = dic->make_unk_pseudo_node_list(start_str + previous_pos, 2, param->unk_max_length);
        find_reached_pos(previous_pos, *node_p);
        set_end_node_list(previous_pos, *node_p);
        return *node_p;
    }
    else {
        return NULL;
    }
}

Node *Sentence::make_unk_pseudo_node_list_from_some_positions(const char *start_str, unsigned int pos, unsigned int previous_pos) {
    Node *node;
    node = dic->make_unk_pseudo_node_list(start_str + pos, 1, param->unk_max_length);
    set_begin_node_list(pos, node);
    find_reached_pos(pos, node);

    // make unknown words from the prevous position
    // if (pos > 0)
    //    make_unk_pseudo_node_list_from_previous_position(start_str, previous_pos);

    return node;
}

Node *Sentence::lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos) {
    return lookup_and_make_special_pseudo_nodes(start_str, pos, 0, NULL);
}

Node *Sentence::lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int specified_length, std::string *specified_pos) {
    return lookup_and_make_special_pseudo_nodes(start_str, 0, specified_length, specified_pos);
}

Node *Sentence::lookup_and_make_special_pseudo_nodes(const char *start_str, unsigned int pos, unsigned int specified_length, std::string *specified_pos) {//{{{
    Node *result_node = NULL;
    Node *kanji_result_node = NULL;
    
    // まず探す
    //Node *dic_node = dic->lookup_lattice(start_str + pos, specified_length, specified_pos); // look up a dictionary with common prefix search
    Node *dic_node = dic->lookup(start_str + pos, specified_length, specified_pos); // look up a dictionary with common prefix search

    // 同じ品詞で同じ長さなら使わない

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
	if (specified_length || pos >= reached_pos_of_pseudo_nodes) {
        // 数詞処理
		// make figure nodes
        // 同じ文字種が続く範囲を一語として入れてくれる
		result_node = dic->make_specified_pseudo_node(start_str + pos,
				specified_length, specified_pos, &(param->unk_figure_pos),
				TYPE_FAMILY_FIGURE);
        // 長さが指定されたものに合っていれば，そのまま返す
		if (specified_length && result_node) return result_node;

		// make alphabet nodes
		if (!result_node) {
			result_node = dic->make_specified_pseudo_node(start_str + pos,
					specified_length, specified_pos, &(param->unk_pos),
					TYPE_FAMILY_ALPH_PUNC);
			if (specified_length && result_node) return result_node;
		}

        // 漢字
        // 辞書にあれば、辞書にない品詞だけを追加する
//        kanji_result_node = dic->make_specified_pseudo_node_by_dic_check(start_str + pos,
//                specified_length, specified_pos, &(param->unk_pos),
//                TYPE_KANJI|TYPE_KANJI_FIGURE, dic_node);
//        if (result_node && kanji_result_node) {
//            //数詞の場合はひとつしか返ってこないことが確定しているので、次のノードに追加すれば大丈夫
//            result_node->bnext = kanji_result_node;
//        }else if(!result_node){
//            result_node = kanji_result_node;
//        }

        // カタカナ
        // TODO:適当に切れるところまでで未定義語を作る
		if (!result_node) {
			result_node = dic->make_specified_pseudo_node_by_dic_check(start_str + pos,
					specified_length, specified_pos, &(param->unk_pos),
					TYPE_KATAKANA , dic_node);
		}

        // ひらがな
//		if (!result_node) {
//			result_node = dic->make_specified_pseudo_node(start_str + pos,
//					3, specified_pos, &(param->unk_pos), TYPE_HIRAGANA );
//			if (specified_length && result_node)
//				return result_node;
//		}

		if (!specified_length && result_node) // only prediction
			find_reached_pos_of_pseudo_nodes(pos, result_node);
	}

    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext)
            tmp_node = tmp_node->bnext;
        tmp_node->bnext = result_node;
        result_node = dic_node;
    }

    return result_node;
}//}}}

Node *Sentence::lookup_and_make_special_pseudo_nodes_lattice(CharLattice &cl, const char *start_str, unsigned int pos, unsigned int specified_length, std::string *specified_pos) {//{{{
    Node *result_node = NULL;
    Node *kanji_result_node = NULL;

    // まず探す
    auto lattice_result = cl.da_search_from_position(dic->darts, pos);
    Node *dic_node = dic->lookup_lattice(lattice_result, start_str + pos, specified_length, specified_pos); // look up a dictionary with common prefix search
    //Node *dic_node = dic->lookup(start_str + pos, specified_length, specified_pos); // look up a dictionary with common prefix search

    // 同じ品詞で同じ長さなら使わない

    // 訓練データで，長さが分かっている場合か，未知語として選択されていない範囲なら
	if (specified_length || pos >= reached_pos_of_pseudo_nodes) {
        // 数詞処理
		// make figure nodes
        // 同じ文字種が続く範囲を一語として入れてくれる
		result_node = dic->make_specified_pseudo_node(start_str + pos,
				specified_length, specified_pos, &(param->unk_figure_pos),
				TYPE_FAMILY_FIGURE);
        // 長さが指定されたものに合っていれば，そのまま返す
		if (specified_length && result_node) return result_node;

		// make alphabet nodes
		if (!result_node) {
			result_node = dic->make_specified_pseudo_node(start_str + pos,
					specified_length, specified_pos, &(param->unk_pos),
					TYPE_FAMILY_ALPH_PUNC);
			if (specified_length && result_node) return result_node;
		}

        // 漢字
        // 辞書にあれば、辞書にない品詞だけを追加する
//        kanji_result_node = dic->make_specified_pseudo_node_by_dic_check(start_str + pos,
//                specified_length, specified_pos, &(param->unk_pos),
//                TYPE_KANJI|TYPE_KANJI_FIGURE, dic_node);
//        if (result_node && kanji_result_node) {
//            //数詞の場合はひとつしか返ってこないことが確定しているので、次のノードに追加すれば大丈夫
//            result_node->bnext = kanji_result_node;
//        }else if(!result_node){
//            result_node = kanji_result_node;
//        }

        // カタカナ
        // TODO:適当に切れるところまでで未定義語を作る
		if (!result_node) {
			result_node = dic->make_specified_pseudo_node_by_dic_check(start_str + pos,
					specified_length, specified_pos, &(param->unk_pos),
					TYPE_KATAKANA , dic_node);
		}

        // ひらがな
//		if (!result_node) {
//			result_node = dic->make_specified_pseudo_node(start_str + pos,
//					3, specified_pos, &(param->unk_pos), TYPE_HIRAGANA );
//			if (specified_length && result_node)
//				return result_node;
//		}

		if (!specified_length && result_node) // only prediction
			find_reached_pos_of_pseudo_nodes(pos, result_node);
	}

    if (dic_node) {
        Node *tmp_node = dic_node;
        while (tmp_node->bnext)
            tmp_node = tmp_node->bnext;
        tmp_node->bnext = result_node;
        result_node = dic_node;
    }

    return result_node;
}//}}}


bool Sentence::lookup_and_analyze() {//{{{
    unsigned int previous_pos = 0;

    // 文字Lattice の構築
    CharLattice cl;
    cl.parse(sentence);

    // lookup あたりを書き換えて，Charlattice 対応版lookup に切り替えるフラグを用意する必要がある
    for (unsigned int pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        if ((*end_node_list)[pos] == NULL) {//pos にたどり着くノードが１つもない
            if (param->unknown_word_detection && pos > 0 && reached_pos <= pos)
                make_unk_pseudo_node_list_from_previous_position(sentence_c_str, previous_pos);
        }
        else {
            //Node *r_node = lookup_and_make_special_pseudo_nodes_lattice(cl,sentence_c_str, pos, 0, NULL); 
            Node *r_node = lookup_and_make_special_pseudo_nodes(sentence_c_str, pos, 0, NULL); 
            // make figure/alphabet nodes and look up a dictionary
            // オノマトペ処理 or lookup_and_make_special_pseudo_nodes 内
            
            set_begin_node_list(pos, r_node);
            find_reached_pos(pos, r_node);
            if (param->unknown_word_detection) { // make unknown word candidates
                if (reached_pos <= pos) {
                    // cerr << ";; Cannot connect at position:" << pos << " (" << in_sentence << ")" << endl;
                    r_node = make_unk_pseudo_node_list_from_some_positions(sentence_c_str, pos, previous_pos);
                }
                else if (r_node && pos >= reached_pos_of_pseudo_nodes) {
                    for (unsigned int i = 2; i <= param->unk_max_length; i++) {
                        r_node = make_unk_pseudo_node_list_by_dic_check(sentence_c_str, pos, r_node, i);
                    }
                    set_begin_node_list(pos, r_node);
                }
            }
            set_end_node_list(pos, r_node);
        }
        previous_pos = pos;
    }
    
    if (param->debug)
        print_juman_lattice();

    // Viterbi
    if(param->nbest){
        for (unsigned int pos = 0; pos < length;
                pos += utf8_bytes((unsigned char *) (sentence_c_str + pos))) {
            viterbi_at_position_nbest(pos, (*begin_node_list)[pos]);
        }

        find_N_best_path();
    }else{
        for (unsigned int pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
            viterbi_at_position(pos, (*begin_node_list)[pos]);
        }
        find_best_path();
    }
    return true;
}//}}}

void Sentence::print_lattice() {
    unsigned int char_num = 0;
    for (unsigned int pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
        while (node) {
            for (unsigned int i = 0; i < char_num; i++)
                cerr << "  ";
            cerr << *(node->string_for_print);
            if (node->string_for_print != node->string)
                cerr << "(" << *(node->string) << ")";
            cerr << "_" << *(node->pos) << endl;
            node = node->bnext;
        }
        char_num++;
    }
}

void Sentence::print_juman_lattice() {
    unsigned int char_num = 0;
    int id = 1;
    // 2 0 0 1 部屋 へや 部屋 名詞 6 普通名詞 1 * 0 * 0 “代表表記:部屋/へや カテゴリ:場所-施設 …"
    // 15 2 2 2 に に に 助詞 9 格助詞 1 * 0 * 0 NIL
    // ID IDs_connecting_from index_begin index_end ... 
        
    std::vector<std::vector<int>> num2id(length); //多めに保持する
    for (unsigned int pos = 0; pos < length; pos += utf8_bytes((unsigned char *)(sentence_c_str + pos))) {
        Node *node = (*begin_node_list)[pos];
            
        while (node) {
            size_t word_length = node->char_num;
            // ID の表示
            if( *node->spos != UNK_POS) { //とりあえず未定義語を出力しない
                cout << id << " " ;
                num2id[char_num + word_length].push_back(id++);
                if(num2id[char_num].size()==0){ // 無かったら 0 を出す
                    cout << "0";
                }else{
                    std::string sep= "";
                    for(auto to_id:num2id[char_num]){
                        cout << sep << to_id;
                        sep = ";";
                    }
                }
                cout << " ";
                // 文字index の表示
                cout << char_num << " " << char_num + word_length -1 << " ";

                // 表層 よみ 原形
                if( *node->reading == UNK_POS ){//読み不明であれば表層を使う
                    cout << *node->original_surface << " " << *node->original_surface << " " << *node->original_surface << " " ; 
                }else{
                    cout << *node->original_surface << " " << *node->reading  << " " << *node->base << " " ; 
                }
                // 品詞 品詞id
                cout << *node->pos << " " << node->posid << " ";
                // 細分類 細分類id
                if( *node->spos == UNK_POS) {
                    cout << "その他 " << node->sposid << " ";
                }else{
                    cout << *node->spos << " " << node->sposid << " ";
                }
                // 活用型 活用型id
                cout << *node->form_type << " " << node->formtypeid << " ";
                // 活用系 活用系id
                cout << *node->form << " " << node->formid << " ";

                // 意味情報を再構築して表示
                if(*node->representation != "*" || *node->semantic_feature != "NIL" ){
                    cout << '"' ;
                    if(*node->representation != UNK_POS)
                        cout << "代表表記:" << *node->representation << " ";  //*ならスキップ
                    if(*node->semantic_feature != "NIL" )
                        cout << *node->semantic_feature; //NILならNIL
                    cout << '"' << endl;
                }else{
                    cout << "NIL" << endl;
                }
            }
            node = node->bnext;
        }
        char_num++;
    }
    cout << "EOS" << endl;
}



Node *Sentence::get_bos_node() {
	Node *bos_node = new Node;
	bos_node->surface = const_cast<const char *>(BOS);
	bos_node->string = new std::string(bos_node->surface);
	bos_node->string_for_print = bos_node->string;
	bos_node->end_string = bos_node->string;
	// bos_node->isbest = 1;
	bos_node->stat = MORPH_BOS_NODE;
	bos_node->posid = MORPH_DUMMY_POS;
	bos_node->pos = new std::string(BOS_STRING);
	bos_node->spos = new std::string(BOS_STRING);
	bos_node->form = new std::string(BOS_STRING);
	bos_node->form_type = new std::string(BOS_STRING);
	bos_node->base = new std::string(BOS_STRING);

    FeatureSet *f = new FeatureSet(ftmpl);
    f->extract_unigram_feature(bos_node);
    bos_node->wcost = f->calc_inner_product_with_weight();
    bos_node->feature = f;

    return bos_node;
}

Node *Sentence::get_eos_node() {
	Node *eos_node = new Node;
	eos_node->surface = const_cast<const char *>(EOS);
	eos_node->string = new std::string(eos_node->surface);
	eos_node->string_for_print = eos_node->string;
	eos_node->end_string = eos_node->string;
	// eos_node->isbest = 1;
	eos_node->stat = MORPH_EOS_NODE;
	eos_node->posid = MORPH_DUMMY_POS;
	eos_node->pos = new std::string(EOS_STRING);
	eos_node->spos = new std::string(EOS_STRING);
	eos_node->form = new std::string(EOS_STRING);
	eos_node->form_type = new std::string(EOS_STRING);
	eos_node->base = new std::string(EOS_STRING);

    FeatureSet *f = new FeatureSet(ftmpl);
    f->extract_unigram_feature(eos_node);
    eos_node->wcost = f->calc_inner_product_with_weight();
    eos_node->feature = f;

    return eos_node;
}

// make EOS node and get the best path
Node *Sentence::find_best_path() {
    (*begin_node_list)[length] = get_eos_node(); // End Of Sentence
    viterbi_at_position(length, (*begin_node_list)[length]);
    return (*begin_node_list)[length];
}

void Sentence::set_begin_node_list(unsigned int pos, Node *new_node) {
    (*begin_node_list)[pos] = new_node;
}

bool Sentence::viterbi_at_position(unsigned int pos, Node *r_node) {
    while (r_node) {
        long best_score = -INT_MAX;
        Node *best_score_l_node = NULL;
        FeatureSet *best_score_bigram_f = NULL;
        Node *l_node = (*end_node_list)[pos];
        while (l_node) {
            FeatureSet *f = new FeatureSet(ftmpl);
            f->extract_bigram_feature(l_node, r_node);
            double bigram_score = f->calc_inner_product_with_weight();
            long score = l_node->cost + bigram_score + r_node->wcost;
            if (score > best_score) {
                best_score_l_node = l_node;
                if (best_score_bigram_f)
                    delete best_score_bigram_f;
                best_score_bigram_f = f;
                best_score = score;
            }
            else {
                delete f;
            }
            l_node = l_node->enext;
        }

        if (best_score_l_node) {
            r_node->prev = best_score_l_node;
            r_node->next = NULL;
            r_node->cost = best_score;
            if (MODE_TRAIN) { // feature collection
                r_node->feature->append_feature(best_score_l_node->feature);
                r_node->feature->append_feature(best_score_bigram_f);
            }
            delete best_score_bigram_f;
        }
        else {
            return false;
        }

        r_node = r_node->bnext;
    }

    return true;
}

bool Sentence::viterbi_at_position_nbest(unsigned int pos, Node *r_node) {//{{{
	while (r_node) {
		std::priority_queue<NbestSearchToken> nodeHeap;
		Node *l_node = (*end_node_list)[pos];
//        cerr << "r_node: " << *r_node->string_for_print << "_" << *r_node->pos << endl;
            
        while (l_node) {
                FeatureSet f(ftmpl);
                f.extract_bigram_feature(l_node, r_node);
                double bigram_score = f.calc_inner_product_with_weight();

                int traceSize = l_node->traceList.size();

                if (traceSize == 0) {
                    long score = l_node->cost + bigram_score + r_node->wcost;
                    NbestSearchToken newSearchToken(score, 0, l_node);
                    nodeHeap.push(newSearchToken);

//                        cerr << *(l_node->string_for_print) 
//                            << "_" << *l_node->pos  
//                            //<< "_" << *(l_node->representation)
//                            << "\t" << *(r_node->string_for_print) 
//                            << "_" << *r_node->pos 
//                            //<< "_" << *(r_node->representation) 
//                            << "\t" << r_node->wcost 
//                            << "\t" << score << "\t" << "rank" << 0 << endl;
                } else {
                    if (traceSize > param->N_redundant)
                        traceSize = param->N_redundant;

                    for (int i = 0; i < traceSize; ++i) {
                        long score = l_node->traceList.at(i).score + bigram_score
                            + r_node->wcost;
                        NbestSearchToken newSearchToken(score, i, l_node);
                        nodeHeap.push(newSearchToken);
//                        cerr << *(l_node->string_for_print) << "_" << *l_node->pos  
//                            //<< "_" << *(l_node->representation)
//                            << "\t" << *(r_node->string_for_print) 
//                            << "_" << *r_node->pos  
//                            //<< "_" << *(r_node->representation) 
//                            << "\t" << r_node->wcost << "\t"
//                            << score << "\t" << "rank" << i << endl;
                    }
                }
            l_node = l_node->enext;
        }

		int heapSize = nodeHeap.size();

		if (heapSize > param->N_redundant)
			heapSize = param->N_redundant;

		for (int i = 0; i < heapSize; ++i) {
			r_node->traceList.push_back(nodeHeap.top());
			nodeHeap.pop();
		}

		if (r_node->traceList.size() > 0) {
			r_node->next = NULL;
			r_node->prev = r_node->traceList.front().prevNode;
			r_node->cost = r_node->traceList.front().score;
		} else {
			return false;
		}

		r_node = r_node->bnext;
	}

	return true;
}//}}}

void Sentence::print_N_best_path() {//{{{

	std::string output_string_buffer;

	unsigned int N_required = param->N;
	unsigned int N_couter = 0;

	unsigned int traceSize = (*begin_node_list)[length]->traceList.size();
	if (traceSize > param->N_redundant) {
		traceSize = param->N_redundant;
	}

	for (int i = 0; i < traceSize; ++i) {
		Node *node = (*begin_node_list)[length];
		std::vector<Node *> result_morphs;

		bool find_bos_node = false;
		int traceRank = i;

		Node* temp_node = NULL;
//		std::string subword_buffer;
		long output_score = (*begin_node_list)[length]->traceList.at(i).score;
//		cerr << node->traceList.at(i).rank << "\t" << node->traceList.at(i).score << endl;

		while (node) {
		    result_morphs.push_back(node);

			if (node->traceList.size() == 0) {
				break;
			}
			node = node->traceList.at(traceRank).prevNode;
			if (node->stat == MORPH_BOS_NODE) {
				find_bos_node = true;
				break;
			} else {
				traceRank = result_morphs.back()->traceList.at(traceRank).rank;
			}
		}

		if (!find_bos_node)
			cerr << ";; cannot analyze:" << sentence << endl;

		size_t printed_num = 0;
		for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin();
				it != result_morphs.rend(); it++) {

			if ((*it)->stat != MORPH_BOS_NODE && (*it)->stat != MORPH_EOS_NODE) {
				if (printed_num++)
					output_string_buffer.append(" ");
					output_string_buffer.append(*(*it)->string_for_print);
					output_string_buffer.append("_");
					output_string_buffer.append(*(*it)->pos);
			}
		}

		std::map<std::string, int>::iterator find_output = nbest_duplicate_filter.find(output_string_buffer);
		if (find_output != nbest_duplicate_filter.end()) {
			//duplicate output
//			cerr << "duplicate: " << output_string_buffer;
//			cerr << " at " << nbest_duplicate_filter[output_string_buffer] << " vs. " << i << endl;
		} else {
			nbest_duplicate_filter.insert(std::make_pair(output_string_buffer, i));
			cout << "#" << output_score << endl;
			cout << output_string_buffer;
			cout << endl;
			++N_couter;
		}

		output_string_buffer.clear();
		if (N_couter >= N_required)
			break;

	}
	cout << endl;
}//}}}


// update end_node_list
void Sentence::set_end_node_list(unsigned int pos, Node *r_node) {
    while (r_node) {
        if (r_node->stat != MORPH_EOS_NODE) {
            unsigned int end_pos = pos + r_node->length;
            r_node->enext = (*end_node_list)[end_pos];
            (*end_node_list)[end_pos] = r_node;
        }
        r_node = r_node->bnext;
    }
}

unsigned int Sentence::find_reached_pos(unsigned int pos, Node *node) {
    while (node) {
        unsigned int end_pos = pos + node->length;
        if (end_pos > reached_pos)
            reached_pos = end_pos;
        node = node->bnext;
    }
    return reached_pos;
}

// reached_pos_of_pseudo_nodes の計算、どこまでの範囲を未定義語として入れたか
unsigned int Sentence::find_reached_pos_of_pseudo_nodes(unsigned int pos, Node *node) {
    while (node) {
        if (node->stat == MORPH_UNK_NODE) {
            unsigned int end_pos = pos + node->length;
            if (end_pos > reached_pos_of_pseudo_nodes)
                reached_pos_of_pseudo_nodes = end_pos;
        }
        node = node->bnext;
    }
    return reached_pos_of_pseudo_nodes;
}

void Sentence::print_best_path() {
    Node *node = (*begin_node_list)[length];
    std::vector<Node *> result_morphs;

    bool find_bos_node = false;
    while (node) {
        if (node->stat == MORPH_BOS_NODE)
            find_bos_node = true;
        result_morphs.push_back(node);
        node = node->prev;
    }

    if (!find_bos_node)
        cerr << ";; cannot analyze:" << sentence << endl;

    size_t printed_num = 0;
    for (std::vector<Node *>::reverse_iterator it = result_morphs.rbegin(); it != result_morphs.rend(); it++) {
        if ((*it)->stat != MORPH_BOS_NODE && (*it)->stat != MORPH_EOS_NODE) {
            if (printed_num++)
                cout << " ";
            (*it)->print();
        }
    }
    cout << endl;
}

void Sentence::minus_feature_from_weight(std::map<std::string, double> &in_feature_weight, size_t factor) {
    Node *node = (*begin_node_list)[length]; // EOS
    node->feature->minus_feature_from_weight(in_feature_weight, factor);
}

void Sentence::minus_feature_from_weight(std::map<std::string, double> &in_feature_weight) {
    minus_feature_from_weight(in_feature_weight, 1);
}

bool Sentence::lookup_gold_data(std::string &word_pos_pair) {
    if (reached_pos < length) {
        cerr << ";; ERROR! Cannot connect at position for gold: " << word_pos_pair << endl;
    }

    std::vector<std::string> line(2);
    if (word_pos_pair == SPACE_AND_NONE) { // Chiense Treebank: _-NONE-
        line[0] = SPACE;
        line[1] = NONE_POS;
    }
    else {
        split_string(word_pos_pair, "_", line);
    }

    Node *r_node = lookup_and_make_special_pseudo_nodes(line[0].c_str(), strlen(line[0].c_str()), &line[1]);
    if (!r_node) {
        r_node = dic->make_unk_pseudo_node_gold(line[0].c_str(), strlen(line[0].c_str()), line[1]);
        cerr << ";; lookup failed in gold data:" <<  line[0] << ":" << line[1] << endl;
    }
    (*begin_node_list)[length] = r_node;
    find_reached_pos(length, r_node);
    viterbi_at_position(length, r_node);
    set_end_node_list(length, r_node);

    add_one_word(line[0]);
    return true;
}

}

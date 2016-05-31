#include "common.h"
#include <stdlib.h>
#include "cmdline.h"
#include "feature.h"
#include "tagger.h"
#include "sentence.h"

#include "rnnlm/rnnlmlib.h"
#include "rnnlm/rnnlmlib_static.h"
#include "rnnlm/rnnlmlib_dynamic.h"

#include <memory.h>

//namespace SRILM {
//#include "srilm/Ngram.h"
//#include "srilm/Vocab.h"
//}

// define された内容の文字列化を行う
#define str_def(s) xstr(s)
#define xstr(s) #s

bool MODE_TRAIN = false;
bool WEIGHT_AVERAGED = false;

std::string get_home_path(){/*{{{*/
    char* phome;
    std::string home_path;
    phome = getenv("HOME");
    if (phome!=NULL)
        home_path = phome;
    return home_path;
}/*}}}*/

std::string get_current_path(){/*{{{*/
    char* phome;
    std::string home_path;
    phome = getenv("PWD");
    if (phome!=NULL)
        home_path = phome;
    return home_path;
}/*}}}*/

std::string read_kknrc(){/*{{{*/
    std::string home_path = get_home_path();
    std::string current_path = get_current_path();
    std::string jumanpprc_path = home_path + "/.jumanpprc";

#ifdef DEFAULT_MODEL_PATH
    std::string default_model_path = str_def(DEFAULT_MODEL_PATH);
#else
    //std::string default_model_path = current_path + "/.jumanpp";
    std::string default_model_path = "/share/tool/kkn/model/latest";
#endif

    FILE *jumanpprc_file = fopen((jumanpprc_path).c_str(), "r");
    if (jumanpprc_file == NULL){
        return default_model_path;
    }else{
        char buffer[1024];
        if(fscanf(jumanpprc_file, "%s", buffer) == 0){
            fprintf(stderr, "WARNING: .jumanpprc file does not contain valid path\n");
            return default_model_path;
        }
        return buffer;
    }
}/*}}}*/

// オプション
void option_proc(cmdline::parser &option, std::string model_path, int argc, char **argv) {//{{{
    std::string bin_path(argv[0]);
    std::string bin_dir =  bin_path.substr(0,bin_path.rfind('/')); // Windows 非対応
    
    // 設定の読み込みオプション
    option.add<std::string>("dir", 'D', "set resource directory", false, model_path);
    option.add<std::string>("dict", 0, "dictionary filepath", false, model_path+"/dic");
    option.add<std::string>("model", 0, "model filepath", false, model_path+"/model.mdl");
    option.add<std::string>("rnnlm", 0, "RNNLM filepath", false, model_path+"/lang.mdl"); 
    option.add<std::string>("feature", 0, "feature template filepath", false, model_path+"/feature.def");
    option.add<std::string>("use_lexical_feature", 0, "set frequent word list for lexical feature",false, model_path+"/freq_words.list"); 

#ifdef USE_SRILM
    option.add<std::string>("srilm", 'I', "srilm filename", false, "srilm.arpa");
#endif
        
    // 解析方式の指定オプション
    option.add<unsigned int>("beam", 'B', "set beam width",false,5);
        
    // 出力形式のオプション
    option.add("juman", 'j', "print juman style (default)"); 
    option.add("morph", 'M', "print morph style");
    option.add<unsigned int>("Nmorph", 'N', "print N-best Moprh", false, 5);
    option.add<unsigned int>("lattice", 'L', "output lattice format",false, 5);
    option.add("noambiguous", 0, "do not output ambiguous words on lattice");
    option.add("oldstyle", 0, "print old style lattice");
        
    // 訓練用オプション
    option.add<std::string>("train", 't', "set training data path", false, "data/train.txt");
    option.add("scw", 0, "use soft confidence weighted in the training");
    option.add("shuffle", 's', "shuffle training data for each iteration"); //デフォルトに
    option.add<unsigned int>("iteration", 'i', "iteration number for training", false, 10);
    option.add<double>("Cvalue", 'C', "C value. parameter for SCW",false, 1.0);
    option.add<double>("Phi", 'P', "Phi value. parameter for SCW",false, 1.65);
        
    // デフォルト化
    // option.add("ambiguous", 'A', "output ambiguous words on lattice (default)");
    //option.add<unsigned int>("unk_max_length", 'l', "maximum length of unknown word detection", false, 2);
        

    option.add<double>("Rweight", 0, "linear interpolation parameter for MA score and RNN score",false, 0.3);
    option.add<double>("Lweight", 0, "linear penalty parameter for unknown words in RNNLM",false, 1.5);
    //option.add<unsigned int>("nbest", 'n', "n-best search", false, 5);
    //option.add<unsigned int>("rerank", 'R', "n-best reranking", false, 5);
    //option.add("oldloss", 0, "use old loss function");
    //option.add<std::string>("lda", 0, "use lda", false, ""); //廃止予定
    //option.add("averaged", 'a', "use averaged perceptron for training");
    //option.add("total", 'T', "use total similarity for LDA");
         
    // 以下は廃止するオプション
    //option.add("passiveunk", '\0', "apply passive unknown word detection. The option use unknown word detection only if it cannot make any node."); 
    //option.add("notrigram", 0, "do NOT use trigram feature");
    //option.add("unknown", 'u', "apply unknown word detection (obsolete; already default)");
    //option.add("part", '\0', "use partical annotation");

    // デバッグオプション
    option.add("debug", '\0', "debug mode");
    option.add("rnndebug", '\0', "show rnnlm debug message");
    option.add("ptest", 0, "receive partially annotated text (dev)");
   
#ifdef USE_DEV_OPTION
    // 開発用オプション
    option.add("nornnlm", 0, "do not use RNNLM");
    option.add("dynamic", 0, "loading RNNLM dynamically (dev)");
    option.add("rnnasfeature", 0, "use rnnlm score as feature (dev)");
    option.add("userep", 0, "use rep in rnnlm (dev)");
    option.add("usebase", 0, "use rep in rnnlm (dev,default)");
    option.add("usesurf", 0, "use surf in rnnlm (dev)");
    option.add("usepos", 0, "use pos in rnnlm (dev,default)");
    option.add("printrep",0, "print rep(dev)");
#endif

    option.add("version", 'v', "print version");
    option.add("help", 'h', "print this message");
    option.parse_check(argc, argv);
    if (option.exist("version")) {//{{{
        cout << "JUMAN++ " << VERSION << "(" << GITVER <<")" << endl;
        exit(0);
    }//}}}
    if (option.exist("train")) {//{{{
        MODE_TRAIN = true;
    }//}}}
}//}}}

// unit_test 時はmainを除く
#ifndef KKN_UNIT_TEST

//終了時にdartsのclearでbus errorが出てる

int main(int argc, char** argv) {//{{{
    cmdline::parser option;
    std::string data_path = read_kknrc();
    option_proc(option, data_path, argc, argv);
    
    // TODO: オプションの処理，初期化，解析を分離する
    std::string rnnlm_model_path = data_path+"/lang.mdl";
    std::string dict_path = data_path +"/dic";
    std::string model_path = data_path + "/model.mdl";
    std::string freq_word_list = data_path + "/freq_words.list";
    std::string feature_path = data_path + "/feature.def";
    if(option.exist("dir")){
        rnnlm_model_path = option.get<std::string>("dir") + "/lang.mdl"; 
        dict_path = option.get<std::string>("dir") + "/dic";
        model_path = option.get<std::string>("dir") + "/model.mdl";
        freq_word_list = option.get<std::string>("dir") + "/freq_words.list";
        feature_path = option.get<std::string>("dir") + "/feature.def";
    }

    if(option.exist("rnnlm"))
        rnnlm_model_path = option.get<std::string>("rnnlm");
    if(option.exist("dict"))
        dict_path = option.get<std::string>("dict");
    if(option.exist("model"))
        model_path = option.get<std::string>("model");
    if(option.exist("use_lexical_feature"))
        freq_word_list = option.get<std::string>("use_lexical_feature");
    if(option.exist("feature"))
        feature_path = option.get<std::string>("feature");
    
    unsigned int unk_max_length = 2; // 固定
    
    Morph::Parameter param( dict_path, feature_path, option.get<unsigned int>("iteration"), true, 
            option.exist("shuffle"), unk_max_length, option.exist("debug"), option.exist("lattice"));

    // モデルパスの設定
    param.set_model_filename(model_path); //訓練用
    param.use_lexical_feature=true;
    param.freq_word_list = freq_word_list;
    Morph::FeatureSet::open_freq_word_set(param.freq_word_list);

    if(param.debug)
        std::cerr << "initializing models ... " << std::flush;
    else
        setenv("TCMALLOC_LARGE_ALLOC_REPORT_THRESHOLD","107374182400",1);

    // beam は default
    param.set_beam(true);
    param.set_N(5); //初期値

    if(option.exist("Nmorph")){
        param.set_N(option.get<unsigned int>("Nmorph"));
    }else if(option.exist("beam")){
        param.set_N(option.get<unsigned int>("beam"));
    }

    if(option.exist("lattice")){ 
        // beam が設定されていたら，lattice のN は表示のみに使う
        if(option.exist("beam")){
            param.set_L(option.get<unsigned int>("lattice"));
        }else{// 無ければラティスと同じ幅を指定
            // 指定されたビーム幅よりもラティスに使うN-best が多ければ，ビーム幅を拡張する．
            if(option.get<unsigned int>("lattice") > param.N)
                param.set_N(option.get<unsigned int>("lattice"));
            param.set_L(option.get<unsigned int>("lattice"));
        }
    }    
    
    param.set_output_ambigous_word(!option.exist("noambiguous"));
    
    // TODO: 訓練はデフォルトでSCWを利用するようにする
    param.set_use_scw(option.exist("scw"));
    param.set_lweight(option.get<double>("Lweight"));
    if(option.exist("Cvalue"))
        param.set_C(option.get<double>("Cvalue"));
    if(option.exist("Phi"))
        param.set_Phi(option.get<double>("Phi"));

    // 部分アノテーション用デリミタ
    param.delimiter = "\t";

#ifdef USE_DEV_OPTION
    if(option.exist("rnnasfeature") && option.exist("rnnlm") && !option.exist("nornnlm")){
        param.use_rnnlm_as_feature = true;
    }
#endif

    if(option.exist("debug")){
        Morph::FeatureSet::debug_flag = true;
    }

    // trigram はdefault
    param.set_trigram(true);
    param.set_rweight(option.get<double>("Rweight"));

    param.use_suu_rule = true;
    param.no_posmatch = true;

    // LDA用、廃止予定
    Morph::Parameter normal_param = param;
    normal_param.set_nbest(true); // nbest を利用するよう設定
    normal_param.set_N(10); // 10-best に設定

// RNNLM の利用フラグ設定    
#ifdef USE_DEV_OPTION
    if(option.exist("userep"))
        param.userep = true;
    if(option.exist("usebase"))
        param.usebase = true;
    if(option.exist("usesurf"))
        param.usesurf = true;
    if(option.exist("usepos"))
        param.usepos = true;

    if(option.exist("nornnlm")){
        param.set_rnnlm(false);
        param.set_nce(false);
    }else{
        param.set_rnnlm(true);
        param.set_nce(true);
    }
#else
    param.set_rnnlm(true);
    param.set_nce(true);
#endif

#ifdef USE_SRILM
    param.set_srilm(option.exist("srilm"));
#endif
    Morph::Tagger tagger(&param);
    Morph::Node::set_param(&param);
   
    RNNLM::CRnnLM* p_rnnlm;
#ifdef USE_DEV_OPTION
    if(option.exist("dynamic")){
        p_rnnlm = new RNNLM::CRnnLM_dyn();
    }else{
        p_rnnlm = new RNNLM::CRnnLM_stat();
    }
#else
    p_rnnlm = new RNNLM::CRnnLM_dyn();
#endif
        
    p_rnnlm->setRnnLMFile(rnnlm_model_path.c_str());
    if(option.exist("rnndebug")){
        p_rnnlm->setDebugMode(1);
        param.rnndebug = true;
    }else
        p_rnnlm->setDebugMode(0);
    if(param.lpenalty)
        p_rnnlm->setLweight(param.lweight);
    srand(1);
    Morph::Sentence::init_rnnlm_FR(p_rnnlm);

#ifdef USE_SRILM /*{{{*/
    Vocab *vocab;
    Ngram *ngramLM;
    if(option.exist("srilm")){//
        vocab = new Vocab;
        vocab->unkIsWord() = true; // use unknown <unk> 
        //File file(vocabFile, "r"); // vocabFile
        //vocab->read(file); //オプションでは指定してない
        ngramLM = new Ngram(*vocab, 3); //order = 3,or 4?

        std::string lmfile = option.get<std::string>("srilm"); // "./srilm.vocab";
        //SRILM::
        File file(lmfile.c_str(), "r");//
        bool limitVocab = false; //?

	    if (!ngramLM->read(file, limitVocab)) {
            cerr << "format error in lm file " << lmfile << endl;
            exit(1);
	    }
        //ngramLM->skipOOVs() = true; 
        //ngramLM->linearPenalty() = true; // 未知語のスコアの付け方を変更

        Morph::Sentence::init_srilm(ngramLM, vocab);
    }
#endif /*}}}*/

//    if (option.exist("ptrain")) {//部分アノテーション学習モード{{{
//        std::cerr << "done" << std::endl;
//        tagger.ptrain(option.get<std::string>("train"));
//        tagger.write_bin_model_file(option.get<std::string>("model"));
//      //}}}
//    } else 
    if (option.exist("ptest")) {//部分アノテーション付き形態素解析{{{
        tagger.read_bin_model_file(model_path);
        std::cerr << "done" << std::endl;
        param.delimiter = "\t";
            
        std::string buffer;
        while (getline(cin, buffer)) {
            tagger.partial_annotation_analyze(buffer);
            tagger.print_best_beam_juman();
            tagger.sentence_clear();
        }
      //}}}
    } else if (MODE_TRAIN) {//学習モード{{{
//        std::cerr << "done" << std::endl;
//        if(option.exist("lda")){
//            std::cerr << "LDA training is obsoleted" << std::endl;
//            return 0;
//            Morph::Tagger tagger_normal(&normal_param);
//            tagger_normal.read_model_file(option.get<std::string>("lda"));
//                
//            tagger.train_lda(option.get<std::string>("train"), tagger_normal);
//            tagger.write_bin_model_file(option.get<std::string>("model"));
//        }else if (option.exist("part")) { //部分的アノテーションからの学習
//            tagger.read_bin_model_file(option.get<std::string>("model"));
//            // diagの読み込み
//            //tagger.ptrain(option.get<std::string>("train"));
//            tagger.write_bin_model_file(option.get<std::string>("model")+"+"); //ココのファイル名はオプションで与えられるようにする
//        }else{ //通常の学習
            tagger.train(option.get<std::string>("train"));
            tagger.write_bin_model_file(model_path);
//        }
      //}}}
//    } else if(option.exist("lda")){//LDAを使う形態素解析{{{
//        std::cerr << "LDA mode is depreciated" << std::endl;
//        return 0;
//        Morph::Tagger tagger_normal(&normal_param);
//        tagger_normal.read_model_file(option.get<std::string>("lda"));
//        tagger.read_model_file(option.get<std::string>("model"));
//        std::cerr << "done" << std::endl;
//            
//        std::ifstream is(argv[1]); // input stream
//            
//        // sentence loop
//        std::string buffer;
//        while (getline(is ? is : cin, buffer)) {
//            if (buffer.length() == 0 || buffer.at(0) == '#') { // empty line or comment line
//                cout << buffer << endl;
//                continue;
//            }
//            Morph::Sentence* normal_sent = tagger_normal.new_sentence_analyze(buffer);
//            TopicVector topic = normal_sent->get_topic();
//
//            //std::cerr << "Topic:";
//            //for(double d: topic){
//            //    std::cerr << d << ",";
//            //}
//            //std::cerr << endl;
//
//            Morph::Sentence* lda_sent = tagger.new_sentence_analyze_lda(buffer, topic);
//            if (option.exist("lattice")){
//                if (option.exist("oldstyle"))
//                    tagger.print_old_lattice();
//                else
//                    tagger.print_lattice();
//            }else{
//                if(option.exist("nbest")){
//                    tagger.print_N_best_path();
//                }else{
//                    tagger.print_best_path();
//                }
//            }
//            delete(normal_sent);
//            delete(lda_sent);
//        }
    //}}}
    } else {// 通常の形態素解析{{{
        tagger.read_bin_model_file(model_path);
        if(param.debug) std::cerr << "done" << std::endl;
            
        std::ifstream is(argv[1]); // input stream
            
        // sentence loop
        std::string buffer;
        while (getline(is ? is : cin, buffer)) {
            if (buffer.length() < 1 ) { // empty line or comment line
                std::cout << buffer << std::endl;
                continue;
            }else if(buffer.at(0) == '#'){
                if(buffer.length() <= 1){
                    std::cout << buffer << " " << VERSION << "(" << GITVER <<")" << std::endl;
                    continue;
                }

                // 動的コマンドの処理
                std::size_t pos;
                if( (pos = buffer.find("##KKN\t")) != std::string::npos ){
                    std::size_t arg_pos;
                    // input:
                    // ##KKN<tab>command arg
                    // ##KKN<tab>setL 5
                        
                    std::string command = "set-lattice";
                    if( (arg_pos = buffer.find(command)) != std::string::npos){
                        arg_pos = buffer.find_first_of(" \t",arg_pos+command.length());
                        long val = std::stol(buffer.substr(arg_pos));
                        param.set_L(val);
                        std::cout << "##KKN\tset-lattice " << val << std::endl;
                    }

                    command = "set-beam";
                    if( (arg_pos = buffer.find(command)) != std::string::npos){
                        arg_pos = buffer.find_first_of(" \t",arg_pos+command.length());
                        long val = std::stol(buffer.substr(arg_pos));
                        param.set_N(val);
                        std::cout << "##KKN\tset-beam " << val << std::endl;
                    }

                    command = "set-ambiguous";
                    if( (arg_pos = buffer.find(command)) != std::string::npos){
                        param.set_output_ambigous_word(true);
                        std::cout << "##KKN\tset-ambiguous " << std::endl;
                    }
                    
                    command = "unset-ambiguous";
                    if( (arg_pos = buffer.find(command)) != std::string::npos){
                        param.set_output_ambigous_word(false);
                        std::cout << "##KKN\tunset-ambiguous " << std::endl;
                    }
                }else{// S-ID の処理
                    std::cout << buffer << " " << VERSION << "(" << GITVER <<")" << std::endl;
                }
                continue;
            }
                
            tagger.new_sentence_analyze(buffer);
            if (option.exist("lattice")){
                if (option.exist("oldstyle"))
                    tagger.print_old_lattice();
                else{
                    // 通常のNbestラティス
                    tagger.print_lattice_rbeam(param.L);
                }
            }else{
                if(option.exist("Nmorph")){
                    // N-best の Moprh形式出力
                    tagger.print_beam();
                }else if(option.exist("morph")){
#if USE_DEV_OPTION
                    if(option.exist("printrep"))
                        tagger.print_best_beam_rep();
                    else
#endif
                        tagger.print_best_beam();//Morph出力
                }else{
                    tagger.print_best_beam_juman();
                }
            }
            tagger.sentence_clear();
        }
    }//}}}
    return 0;
}//}}}



#endif

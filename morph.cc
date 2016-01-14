#include "common.h"
#include "cmdline.h"
#include "feature.h"
#include "tagger.h"
#include "sentence.h"
#include <memory.h>

//namespace SRILM {
//#include "srilm/Ngram.h"
//#include "srilm/Vocab.h"
//}

bool MODE_TRAIN = false;
bool WEIGHT_AVERAGED = false;

// オプション
void option_proc(cmdline::parser &option, int argc, char **argv) {//{{{
    std::string bin_path(argv[0]);
    std::string bin_dir =  bin_path.substr(0,bin_path.rfind('/')); // Unix依存
    
    // 設定の読み込みオプション
    option.add<std::string>("dir", 'D', "set model directory", false, bin_dir+"data");

    option.add<std::string>("dict", 0, "dict filename", false, "data/japanese.dic");
    option.add<std::string>("model", 0, "model filename", false, "data/model.mdl");
    option.add<std::string>("rnnlm", 0, "rnnlm filename", false, "data/lang.mdl"); 
    option.add<std::string>("feature", 0, "feature template filename", false, "data/feature.def");
    option.add<std::string>("use_lexical_feature", 0, "change frequent word list for lexical feature",false,"data/freq_words.list"); 

#ifdef USE_SRILM
    option.add<std::string>("srilm", 'I', "srilm filename", false, "srilm.arpa");
#endif
        
    // 解析方式の指定オプション
    option.add<unsigned int>("beam", 'B', "set beam width (default 5)",false,5);

    // 出力形式のオプション
    option.add("juman", 'j', "print juman style (default)"); 
    option.add("morph", 'M', "print morph style");
    option.add<unsigned int>("Nmorph", 'N', "print N-best Moprh",false,5);
    option.add<unsigned int>("lattice", 'L', "output lattice format",false, 5);
    option.add("ambiguous", 'A', "output ambiguous words on lattice");
    option.add("oldstyle", 0, "print old style lattice");

    // 訓練用オプション
    option.add<std::string>("train", 't', "training filename", false, "data/train.txt");
    option.add("scw", 0, "use soft confidence weighted");
    option.add("shuffle", 's', "shuffle training data for each iteration"); //デフォルトに
    //option.add<std::string>("ptrain", 'p', "partial training filename", false, "data/ptrain.txt");
    option.add<unsigned int>("iteration", 'i', "iteration number for training", false, 10);
    option.add<double>("Cvalue", 'C', "C value",false, 1.0);
    option.add<double>("Phi", 'P', "Phi value",false, 1.65);
        
    // デフォルト化
    //option.add<unsigned int>("unk_max_length", 'l', "maximum length of unknown word detection", false, 2);

    option.add<double>("Rweight", 0, "linear interpolation parameter for KKN score and RNN score (default=0.3)",false, 0.3);
    option.add<double>("Lweight", 0, "linear penalty parameter for unknown words in RNNLM (default=1.5)",false, 1.5);
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
    
    // 開発用オプション
    option.add("ptest", 0, "receive partially annotated text (dev)");
    option.add("rnnasfeature", 0, "use rnnlm score as feature (dev)");
    option.add("userep", 0, "use rep in rnnlm (dev)");
    option.add("printrep",0, "print rep(dev)");

    option.add("version", 'v', "print version");
    option.add("help", 'h', "print this message");
    option.parse_check(argc, argv);
    if (option.exist("version")) {//{{{
        cout << "KKN " << VERSION << "(" << GITVER <<")" << endl;
        exit(0);
    }//}}}
    if (option.exist("train")) {//{{{
        MODE_TRAIN = true;
    }//}}}
//    if (option.exist("averaged")) {//{{{
//        WEIGHT_AVERAGED = true;
//    }//}}}
}//}}}

// unit_test 時はmainを除く
#ifndef KKN_UNIT_TEST

//終了時にdartsのclearでbus errorが出てる

int main(int argc, char** argv) {//{{{
    cmdline::parser option;
    option_proc(option, argc, argv);


    // モデルパス
    std::string rnnlm_model_path = "data/lang.mdl";
    std::string dict_path = "data/japanese.dic";
    std::string model_path = "data/model.mdl";
    std::string freq_word_list = "data/freq_words.list";
    std::string feature_path = "data/feature.def";
    if(option.exist("dir")){
        //std::cout << option.get<std::string>("dir") << std::endl;
        rnnlm_model_path = option.get<std::string>("dir") + "/lang.mdl"; 
        dict_path = option.get<std::string>("dir") + "/japanese.dic";
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
    
    //option.get<unsigned int>("unk_max_length")
    unsigned int unk_max_length = 2; // 固定
    
    Morph::Parameter param( dict_path, feature_path, option.get<unsigned int>("iteration"), true, 
            option.exist("shuffle"), unk_max_length, option.exist("debug"), option.exist("lattice"));

    // モデルパスの設定
    param.set_model_filename(model_path);
    param.use_lexical_feature=true;
    param.freq_word_list = option.get<std::string>("use_lexical_feature");
    Morph::FeatureSet::open_freq_word_set(param.freq_word_list);


    // 基本的にbeam は使う
    param.set_beam(true);
    param.set_N(5); //初期値

    // beam, rbeam 以外のオプションを廃止予定
    //if(option.exist("nbest"))
    //    param.set_N(option.get<unsigned int>("nbest"));
    //else 
    if(option.exist("Nmorph")){
        param.set_N(option.get<unsigned int>("Nmorph"));
    }else if(option.exist("beam")){
        param.set_N(option.get<unsigned int>("beam"));
    }//else{
    //    param.set_N(1);
    //}

    if(option.exist("lattice")){ 
        // beam が設定されていたら，lattice のN は表示のみに使う
        if(option.exist("beam")){
            param.L = option.get<unsigned int>("lattice");
        }else{// 無ければラティスと同じ幅を指定
            param.set_N(option.get<unsigned int>("lattice"));
            param.L = option.get<unsigned int>("lattice");
        }
    }    
    
    param.set_output_ambigous_word(option.exist("ambiguous"));
//    if(option.exist("passiveunk"))
//        param.set_passive_unknown(true);
    param.set_use_scw(option.exist("scw"));
    param.set_lweight(option.get<double>("Lweight"));
    if(option.exist("Cvalue"))
        param.set_C(option.get<double>("Cvalue"));
    if(option.exist("Phi"))
        param.set_Phi(option.get<double>("Phi"));
//    if(option.exist("total")){ // LDA用, 廃止予定
//        param.set_use_total_sim();
//        Morph::FeatureSet::use_total_sim = true;
//    }
    // 部分アノテーション用デリミタ
    param.delimiter = "\t";

    if(option.exist("rnnasfeature") && option.exist("rnnlm")){
        param.use_rnnlm_as_feature = true;
    }

    if(option.exist("debug")){
        Morph::FeatureSet::debug_flag = true;
    }

    // trigram は基本的に使う
    //param.set_trigram(!option.exist("notrigram"));
    param.set_trigram(true);
    param.set_rweight(option.get<double>("Rweight"));

//    if(option.exist("oldloss")){ //廃止予定
//        param.useoldloss = true;
//    }
    param.use_suu_rule = true;
    param.no_posmatch = true;

    // LDA用、廃止予定
    Morph::Parameter normal_param = param;
    normal_param.set_nbest(true); // nbest を利用するよう設定
    normal_param.set_N(10); // 10-best に設定

    param.set_rnnlm(option.exist("rnnlm"));
    param.set_nce(option.exist("rnnlm"));
    
    if(option.exist("userep"))
        param.userep = true;

#ifdef USE_SRILM
    param.set_srilm(option.exist("srilm"));
#endif
    Morph::Tagger tagger(&param);
    Morph::Node::set_param(&param);
   
    RNNLM::CRnnLM rnnlm;
    if(option.exist("rnnlm")){
        rnnlm.setLambda(1.0);
        rnnlm.setRegularization(0.0000001);
        rnnlm.setDynamic(0);
        rnnlm.setRnnLMFile(rnnlm_model_path.c_str());
        rnnlm.setRandSeed(1);
        rnnlm.useLMProb(0);
        if(option.exist("rnndebug"))
            rnnlm.setDebugMode(1);
        else
            rnnlm.setDebugMode(0);
        if(param.lpenalty)
            rnnlm.setLweight(param.lweight);
        srand(1);
        Morph::Sentence::init_rnnlm_FR(&rnnlm);
    } 
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
        tagger.read_bin_model_file(option.get<std::string>("model"));
        std::cerr << "done" << std::endl;
        param.delimiter = "\t";
            
        std::string buffer;
        while (getline(cin, buffer)) {
            
            //std::cerr << "input:" << buffer << std::endl;
            Morph::Sentence* pa_sent = tagger.partial_annotation_analyze(buffer);
        
            //if(option.exist("juman"))
                tagger.print_best_beam_juman();
            //else
            //    tagger.print_best_beam();
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
            tagger.write_bin_model_file(option.get<std::string>("model"));
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
        tagger.read_bin_model_file(option.get<std::string>("model"));
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
                    std::cout << buffer << std::endl;
                    continue;
                }

                // 動的コマンドの処理
                std::size_t pos;
                if( (pos = buffer.find("##KKN\t")) != std::string::npos ){
                    std::size_t arg_pos;
                    // input:
                    // ##KKN<tab>command arg
                    // ##KKN<tab>setL 5
                        
                    // setL command
                    std::string command = "setL";
                    if( (arg_pos = buffer.find("setL")) != std::string::npos){
                        arg_pos = buffer.find_first_of(" \t",arg_pos+command.length());
                        long val = std::stol(buffer.substr(arg_pos));
                        param.L = val;
                        std::cout << "##KKN\tsetL " << val << std::endl;
                    }

                    // setN command
                    command = "setN";
                    if( (arg_pos = buffer.find("setN")) != std::string::npos){
                        arg_pos = buffer.find_first_of(" \t",arg_pos+command.length());
                        long val = std::stol(buffer.substr(arg_pos));
                        param.N = val;
                        std::cout << "##KKN\tsetN " << val << std::endl;
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
                    if(option.exist("printrep"))
                        tagger.print_best_beam_rep();
                    else
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

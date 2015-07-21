#include "common.h"
#include "cmdline.h"
#include "feature.h"
#include "tagger.h"
#include "sentence.h"
#include <memory.h>

//namespace SRILM {
#include "srilm/Ngram.h"
#include "srilm/Vocab.h"
//}

bool MODE_TRAIN = false;
bool WEIGHT_AVERAGED = false;

void option_proc(cmdline::parser &option, int argc, char **argv) {//{{{
    option.add<std::string>("dict", 'd', "dict filename", false, "data/japanese_uniq.dic");
    option.add<std::string>("model", 'm', "model filename", false, "data/model.dat");
    option.add<std::string>("binmodel", '\0', "bin model filename", false, "data/model.dat");
    option.add<std::string>("feature", 'f', "feature template filename", false, "data/feature.def");
    option.add<std::string>("train", 't', "training filename", false, "data/train.txt");
    option.add<std::string>("rnnlm", 'r', "rnnlm filename", false, "data/rnnlm.model");
    option.add<std::string>("srilm", 'I', "srilm filename", false, "srilm.arpa");
    option.add<unsigned int>("iteration", 'i', "iteration number for training", false, 10);
    option.add<unsigned int>("unk_max_length", 'l', "maximum length of unknown word detection", false, 7);
    option.add<unsigned int>("lattice", 'L', "output lattice format",false, 1);
    option.add<double>("Cvalue", 'C', "C value",false, 1.0);
    option.add<double>("Phi", 'P', "Phi value",false, 1.65);
    option.add<double>("Rweight", 0, "linear interpolation parameter for KKN score and RNN score",false, 0.5);
    option.add("lpenalty", 0, "use linear penalty");
    option.add<double>("Lweight", 0, "linear penalty parameter for unknown words in RNNLM",false, 1.0);
    option.add<unsigned int>("nbest", 'n', "n-best search", false, 5);
    option.add<unsigned int>("rerank", 'R', "n-best reranking", false, 5);
    option.add("scw", 0, "use soft confidence weighted");
    option.add("so", 0, "use second order viterbi algorithm");
    option.add("trigram", 0, "use trigram feature");
    option.add<std::string>("lda", 0, "use lda", false, "");
    option.add<unsigned int>("beam", 'b', "use beam search",false,1);
    option.add<unsigned int>("rbeam", 'B', "use beam search and reranking",false,1);
    option.add("oldstyle", 'o', "print old style lattice");
    option.add("juman", 'j', "print juman style");
    option.add("averaged", 'a', "use averaged perceptron for training");
    option.add("total", 'T', "use total similarity for LDA");
    option.add("ambiguous", 'A', "output ambiguous words on lattice");
    option.add("shuffle", 's', "shuffle training data for each iteration");
    option.add("unknown", 'u', "apply unknown word detection (obsolete; already default)");
    option.add("passiveunk", '\0', "apply passive unknown word detection. The option use unknown word detection only if it cannot make any node.");
    option.add("debug", '\0', "debug mode");
    option.add("version", 'v', "print version");
    option.add("help", 'h', "print this message");
    option.parse_check(argc, argv);
    if (option.exist("version")) {//{{{
        cout << "KKN " << VERSION << endl;
        exit(0);
    }//}}}
    if (option.exist("train")) {//{{{
        MODE_TRAIN = true;
    }//}}}
    if (option.exist("averaged")) {//{{{
        WEIGHT_AVERAGED = true;
    }//}}}
}//}}}

// unit_test 時はmainを除く
#ifndef KKN_UNIT_TEST

//終了時にdartsのclearでbus errorが出てる

int main(int argc, char** argv) {//{{{
    cmdline::parser option;
    option_proc(option, argc, argv);

    std::cerr << "initializing models ... " << std::flush;

    Morph::Parameter param(option.get<std::string>("dict"), option.get<std::string>("feature"), option.get<unsigned int>("iteration"), true, option.exist("shuffle"), option.get<unsigned int>("unk_max_length"), option.exist("debug"), option.exist("nbest")|option.exist("lattice"));

    if(option.exist("nbest"))
        param.set_N(option.get<unsigned int>("nbest"));
    else if(option.exist("so")){
        param.set_so(true);
    }else if(option.exist("beam")){
        param.set_beam(true);
        param.set_N(option.get<unsigned int>("beam"));
    }else if(option.exist("rbeam")){
        param.set_beam(true);
        param.set_N(option.get<unsigned int>("rbeam"));
    }else if(option.exist("lattice")) // rbeam が設定されていたら，lattice のN は表示のみに使う
        param.set_N(option.get<unsigned int>("lattice"));
    else if(option.exist("rerank"))
        param.set_N(option.get<unsigned int>("rerank"));
    else
        param.set_N(1);

    param.set_output_ambigous_word(option.exist("ambiguous"));
    if(option.exist("passiveunk"))
        param.set_passive_unknown(true);
    param.set_model_filename(option.get<std::string>("model"));
    param.set_use_scw(option.exist("scw"));
    param.set_lpenalty(option.exist("lpenalty")); //矛盾している場合、lweight が優先する
    if(option.exist("Lweight"))
        param.set_lweight(option.get<double>("Lweight"));
    if(option.exist("Cvalue"))
        param.set_C(option.get<double>("Cvalue"));
    if(option.exist("Phi"))
        param.set_Phi(option.get<double>("Phi"));
    if(option.exist("total")){
        param.set_use_total_sim();
        Morph::FeatureSet::use_total_sim = true;
    }

    param.set_trigram(option.exist("trigram"));
    param.set_rweight(option.get<double>("Rweight"));

    Morph::Parameter normal_param = param;
    normal_param.set_nbest(true); // nbest を利用するよう設定
    normal_param.set_N(10); // 10-best に設定
    param.set_rnnlm(option.exist("rnnlm"));
    param.set_srilm(option.exist("srilm"));
    Morph::Tagger tagger(&param);
    Morph::Node::set_param(&param);
   
    RNNLM::CRnnLM rnnlm;
    if(option.exist("rnnlm")){
        rnnlm.setLambda(1.0);
        rnnlm.setRegularization(0.0000001);
        rnnlm.setDynamic(0);
        //rnnlm.setTestFile(test_file);
        rnnlm.setRnnLMFile(option.get<std::string>("rnnlm").c_str());
        rnnlm.setRandSeed(1);
        rnnlm.useLMProb(0);
        rnnlm.setDebugMode(0);
        if(param.lpenalty)
            rnnlm.setLweight(param.lweight);
        srand(1);
        //rnnlm.initialize_test_sent();
        Morph::Sentence::init_rnnlm(&rnnlm);
    }

    //SRILM::
        Vocab *vocab;
    //SRILM::
        Ngram *ngramLM;
    if(option.exist("srilm")){//{{{
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

//        std::cerr << "num_vocab: " << vocab->numWords() << endl;
//        std::cerr << "ind(人): " << vocab->getIndex("人") << std::endl;
//        std::cerr << "word(ind(人)): " << vocab->getWord(vocab->getIndex("人")) << std::endl;
//        
//        std::cerr << "ind(人): " << vocab->getIndex("人") << std::endl;
//        std::cerr << "word(ind(人)): " << vocab->getWord(vocab->getIndex("人")) << std::endl;

        //std::make_unique<SRILM::Vocab>(SRILM::Vocab());
        //std::make_unique<Ngram::Ngram> srilm(Ngram::Ngram())
    }//}}}

    if (MODE_TRAIN) {//学習モード{{{
        std::cerr << "done" << std::endl;
        if(option.exist("lda")){
            Morph::Tagger tagger_normal(&normal_param);
            tagger_normal.read_model_file(option.get<std::string>("lda"));

            tagger.train_lda(option.get<std::string>("train"), tagger_normal);
            tagger.write_model_file(option.get<std::string>("model"));
        }else{ //通常の学習
            tagger.train(option.get<std::string>("train"));
            tagger.write_model_file(option.get<std::string>("model"));
        }
      //}}}
    } else if(option.exist("lda")){//LDAを使う形態素解析{{{
        Morph::Tagger tagger_normal(&normal_param);
        tagger_normal.read_model_file(option.get<std::string>("lda"));
        tagger.read_model_file(option.get<std::string>("model"));
        std::cerr << "done" << std::endl;
            
        std::ifstream is(argv[1]); // input stream
            
        // sentence loop
        std::string buffer;
        while (getline(is ? is : cin, buffer)) {
            if (buffer.length() == 0 || buffer.at(0) == '#') { // empty line or comment line
                cout << buffer << endl;
                continue;
            }
            Morph::Sentence* normal_sent = tagger_normal.new_sentence_analyze(buffer);
            TopicVector topic = normal_sent->get_topic();

            //std::cerr << "Topic:";
            //for(double d: topic){
            //    std::cerr << d << ",";
            //}
            //std::cerr << endl;

            Morph::Sentence* lda_sent = tagger.new_sentence_analyze_lda(buffer, topic);
            if (option.exist("lattice")){
                if (option.exist("oldstyle"))
                    tagger.print_old_lattice();
                else
                    tagger.print_lattice();
            }else{
                if(option.exist("nbest")){
                    tagger.print_N_best_path();
                }else{
                    tagger.print_best_path();
                }
            }
            delete(normal_sent);
            delete(lda_sent);
        }
    //}}}
    } else {// 通常の形態素解析{{{
        //std::cerr << "read model file" << std::flush;
        if(option.exist("binmodel"))
            tagger.read_bin_model_file(option.get<std::string>("binmodel"));
        else
            tagger.read_model_file(option.get<std::string>("model"));
        std::cerr << "done" << std::endl;
            
        std::ifstream is(argv[1]); // input stream
            
        // sentence loop
        std::string buffer;
        while (getline(is ? is : cin, buffer)) {
            if (buffer.length() <= 1 || buffer.at(0) == '#') { // empty line or comment line
                cout << buffer << endl;
                continue;
            }
            tagger.new_sentence_analyze(buffer);
            if (option.exist("lattice")){
                if(option.exist("rbeam")){
                    tagger.print_lattice_rbeam(option.get<unsigned int>("lattice"));
                }else{
                    if (option.exist("oldstyle"))
                        tagger.print_old_lattice();
                    else
                        tagger.print_lattice();
                }
            }else{
                if(option.exist("beam")){
                    tagger.print_beam();
                }else if(option.exist("rbeam")){
                    if(option.exist("juman"))
                        tagger.print_best_beam_juman();
                    else
                        tagger.print_best_beam();
                }else if(option.exist("nbest") && option.exist("rnnlm")){
                    tagger.print_N_best_with_rnn(rnnlm);
                }else if(option.exist("nbest") && !option.exist("rnnlm")){
                    tagger.print_N_best_path();
                }else if(option.exist("rerank") && option.exist("rnnlm")){
                    tagger.print_best_path_with_rnn(rnnlm);
                }else{
                    if(option.exist("juman"))
                        tagger.print_best_beam_juman();
                    else
                        tagger.print_N_best_path();
                }
            }
            tagger.sentence_clear();
        }
    }//}}}
    return 0;
}//}}}

#endif

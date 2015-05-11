#include "common.h"
#include "feature.h"
#include "tagger.h"
#include "cmdline.h"
#include "sentence.h"

bool MODE_TRAIN = false;
bool WEIGHT_AVERAGED = false;

void option_proc(cmdline::parser &option, int argc, char **argv) {//{{{
    option.add<std::string>("dict", 'd', "dict filename", false, "data/japanese_uniq.dic");
    option.add<std::string>("model", 'm', "model filename", false, "data/model.dat");
    option.add<std::string>("feature", 'f', "feature template filename", false, "data/feature.def");
    option.add<std::string>("train", 't', "training filename", false, "data/train.txt");
    option.add<unsigned int>("iteration", 'i', "iteration number for training", false, 10);
    option.add<unsigned int>("unk_max_length", 'l', "maximum length of unknown word detection", false, 7);
    option.add<unsigned int>("lattice", 'L', "output lattice format",false, 1);
    option.add<double>("Cvalue", 'C', "C value",false, 1.0);
    option.add<double>("Phi", 'P', "Phi value",false, 1.65);
    option.add<unsigned int>("nbest", 'n', "n-best search", false, 5);
    option.add("scw", 0, "use soft confidence weighted");
    option.add<std::string>("lda", 0, "use lda", false, "");
    option.add("oldstyle", 'o', "print old style lattice");
    option.add("averaged", 'a', "use averaged perceptron for training");
    option.add("total", 'T', "use total similarity for LDA");
    option.add("ambiguous", 'A', "output ambiguous words on lattice");
    option.add("shuffle", 's', "shuffle training data for each iteration");
    option.add("unknown", 'u', "apply unknown word detection (obsolete; already default)");
    option.add("debug", '\0', "debug mode");
    option.add("version", 'v', "print version");
    option.add("help", 'h', "print this message");
    option.parse_check(argc, argv);
    if (option.exist("version")) {
        cout << "KKN " << VERSION << endl;
        exit(0);
    }
    if (option.exist("train")) {
        MODE_TRAIN = true;
    }
    if (option.exist("averaged")) {
        WEIGHT_AVERAGED = true;
    }
}//}}}

// unit_test 時はmainを除く
#ifndef KKN_UNIT_TEST

int main(int argc, char** argv) {//{{{
    cmdline::parser option;
    option_proc(option, argc, argv);

    Morph::Parameter param(option.get<std::string>("dict"), option.get<std::string>("feature"), option.get<unsigned int>("iteration"), true, option.exist("shuffle"), option.get<unsigned int>("unk_max_length"), option.exist("debug"), option.exist("nbest")|option.exist("lattice"));

    if(option.exist("nbest"))
        param.set_N(option.get<unsigned int>("nbest"));
    else if(option.exist("lattice"))
        param.set_N(option.get<unsigned int>("lattice"));
    else
        param.set_N(1);
    param.set_output_ambigous_word(option.exist("ambiguous"));
    param.set_model_filename(option.get<std::string>("model"));
    param.set_use_scw(option.exist("scw"));
    if(option.exist("Cvalue"))
        param.set_C(option.get<double>("Cvalue"));
    if(option.exist("Phi"))
        param.set_Phi(option.get<double>("Phi"));
    //param.set_debug(option.exist("debug"));
    if(option.exist("total")){
        param.set_use_total_sim();
        Morph::FeatureSet::use_total_sim = true;
    }

    Morph::Parameter normal_param = param;
    normal_param.set_nbest(true);// nbest を利用するよう設定
    normal_param.set_N(5);//5-best に設定
    Morph::Tagger tagger(&param);
    

    if (MODE_TRAIN) {//学習モード{{{
        if(option.exist("lda")){
            Morph::Tagger tagger_normal(&normal_param);
            tagger_normal.read_model_file(option.get<std::string>("lda"));

            tagger.train_lda(option.get<std::string>("train"), tagger_normal);
            tagger.write_model_file(option.get<std::string>("model"));
        }else{
            tagger.train(option.get<std::string>("train"));
            tagger.write_model_file(option.get<std::string>("model"));
        }
      //}}}
    } else if(option.exist("lda")){//LDAを使う形態素解析{{{
        Morph::Tagger tagger_normal(&normal_param);
        tagger_normal.read_model_file(option.get<std::string>("lda"));
        tagger.read_model_file(option.get<std::string>("model"));
            
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
        tagger.read_model_file(option.get<std::string>("model"));
            
        std::ifstream is(argv[1]); // input stream
            
        // sentence loop
        std::string buffer;
        while (getline(is ? is : cin, buffer)) {
            if (buffer.length() == 0 || buffer.at(0) == '#') { // empty line or comment line
                cout << buffer << endl;
                continue;
            }
            tagger.new_sentence_analyze(buffer);
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
            tagger.sentence_clear();
        }
    }//}}}
    return 0;
}//}}}

#endif

#include "common.h"
#include "tagger.h"
#include "cmdline.h"

bool MODE_TRAIN = false;
bool WEIGHT_AVERAGED = false;
//std::unordered_map<std::string, double> feature_weight;
//FeatureVector feature_weight; // -> Tagger に移さないと複数のモデルを扱えない
//std::unordered_map<std::string, double> feature_weight_sum;

void option_proc(cmdline::parser &option, int argc, char **argv) {
    option.add<std::string>("dict", 'd', "dict filename", false, "data/japanese_uniq.dic");
    //option.add<std::string>("bin", 'b', "dicbin filename", false, "data/dic.bin");
    //option.add<std::string>("pos", 'p', "POS list filename", false, "data/dic.pos");
    option.add<std::string>("model", 'm', "model filename", false, "data/model.dat");
    option.add<std::string>("feature", 'f', "feature template filename", false, "data/feature.def");
    option.add<std::string>("train", 't', "training filename", false, "data/train.txt");
    option.add<unsigned int>("iteration", 'i', "iteration number for training", false, 10);
    option.add<unsigned int>("unk_max_length", 'l', "maximum length of unknown word detection", false, 7);
    option.add<unsigned int>("lattice", 'L', "output lattice format",false, 1);
    option.add<double>("Cvalue", 'C', "C value",false, 1.0);
    option.add<double>("Phi", 'P', "Phi value",false, 1.65);
    option.add("nbest", 'n', "n-best search");
    option.add("scw", 0, "use soft confidence weighted");
    option.add("oldstyle", 'o', "print old style lattice");
    option.add("averaged", 'a', "use averaged perceptron for training");
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
}

// write feature weights
bool write_model_file(const std::string &model_filename) {
    std::ofstream model_out(model_filename.c_str(), std::ios::out);
    if (!model_out.is_open()) {
        cerr << ";; cannot open " << model_filename << " for writing" << endl;
        return false;
    }
    for (std::unordered_map<std::string, double>::iterator it = feature_weight.begin(); it != feature_weight.end(); it++) {
        model_out << it->first << " " << it->second << endl;
    }
    model_out.close();
    return true;
}

// read feature weights
bool read_model_file(const std::string &model_filename) {
    std::ifstream model_in(model_filename.c_str(), std::ios::in);
    if (!model_in.is_open()) {
        cerr << ";; cannot open " << model_filename << " for reading" << endl;
        exit(1);
    }
    std::string buffer;
    while (getline(model_in, buffer)) {
        std::vector<std::string> line;
        Morph::split_string(buffer, " ", line);
        feature_weight[line[0]] = atof(static_cast<const char *>(line[1].c_str()));
    }
    model_in.close();
    return true;
}

// unit_test 時はmainを除く
#ifndef KKN_UNIT_TEST

int main(int argc, char** argv) {
    cmdline::parser option;
    option_proc(option, argc, argv);

    Morph::Parameter param(option.get<std::string>("dict"), option.get<std::string>("feature"), option.get<unsigned int>("iteration"), true, option.exist("shuffle"), option.get<unsigned int>("unk_max_length"), option.exist("debug"), option.exist("nbest")|option.exist("lattice"));
    param.set_N(option.get<unsigned int>("lattice"));
    param.set_output_ambigous_word(option.exist("ambiguous"));
    param.set_model_filename(option.get<std::string>("model"));
    param.set_use_scw(option.exist("scw"));
    if(option.exist("Cvalue"))
        param.set_C(option.get<double>("Cvalue"));
    if(option.exist("Phi"))
        param.set_Phi(option.get<double>("Phi"));
    //param.set_debug(option.exist("debug"));
    Morph::Tagger tagger(&param);

    if (MODE_TRAIN) {
        tagger.train(option.get<std::string>("train"));
        write_model_file(option.get<std::string>("model"));
        feature_weight_sum.clear();
    } else {
        read_model_file(option.get<std::string>("model"));
            
        std::ifstream is(argv[1]); // input stream
            
        // sentence loop
        std::string buffer;
        while (getline(is ? is : cin, buffer)) {
            if (buffer.length() == 0 || buffer.at(0) == '#') { // empty line or comment line
                cout << buffer << endl;
                continue;
            }
            tagger.new_sentence_analyze(buffer);
            if (option.exist("lattice"))
                if (option.exist("oldstyle"))
                    tagger.print_old_lattice();
                else
                    tagger.print_lattice();
            else{
                if(option.exist("nbest")){
                    tagger.print_N_best_path();
                }else{
                    tagger.print_best_path();
                }
            }
            tagger.sentence_clear();
        }
    }

    feature_weight.clear();
    return 0;
}

#endif

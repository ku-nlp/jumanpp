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

// namespace SRILM {
//#include "srilm/Ngram.h"
//#include "srilm/Vocab.h"
//}

// define された内容の文字列化を行うマクロ
#define str_def(s) xstr(s)
#define xstr(s) #s

bool MODE_TRAIN = false;
bool WEIGHT_AVERAGED = false;

std::string get_home_path() { /*{{{*/
    char *phome;
    std::string home_path;
    phome = getenv("HOME");
    if (phome != NULL)
        home_path = phome;
    return home_path;
} /*}}}*/

std::string get_current_path() { /*{{{*/
    char *phome;
    std::string home_path;
    phome = getenv("PWD");
    if (phome != NULL)
        home_path = phome;
    return home_path;
} /*}}}*/

std::string read_jumanpprc() { /*{{{*/
    std::string home_path = get_home_path();
    std::string current_path = get_current_path();
    std::string jumanpprc_path = home_path + "/.jumanpprc";

#ifdef DEFAULT_MODEL_PATH
    std::string default_model_path = str_def(DEFAULT_MODEL_PATH);
#else
    std::string default_model_path = current_path + "/.jumanpp";
#endif

    FILE *jumanpprc_file = fopen((jumanpprc_path).c_str(), "r");
    if (jumanpprc_file == NULL) {
        return default_model_path;
    } else {
        char buffer[1024];
        if (fscanf(jumanpprc_file, "%s", buffer) == 0) {
            fprintf(stderr,
                    "WARNING: .jumanpprc file does not contain valid path\n");
            return default_model_path;
        }
        return buffer;
    }
} /*}}}*/

// オプション
void option_proc(cmdline::parser &option, std::string model_path, int argc,
                 char **argv) { //{{{
    std::string bin_path(argv[0]);
    std::string bin_dir =
        bin_path.substr(0, bin_path.rfind('/')); // Windows 非対応

    // 設定の読み込みオプション
    option.add<std::string>("dir", 'D', "set resource directory", false,
                            model_path);
    option.add<std::string>("dict", 0, "dictionary filepath", false,
                            model_path + "/dic");
    option.add<std::string>("model", 0, "model filepath", false,
                            model_path + "/model.mdl");
    option.add<std::string>("rnnlm", 0, "RNNLM filepath", false,
                            model_path + "/lang.mdl");
    option.add<std::string>("feature", 0, "feature template filepath", false,
                            model_path + "/feature.def");
    option.add<std::string>("use_lexical_feature", 0,
                            "set frequent word list for lexical feature", false,
                            model_path + "/freq_words.list");

#ifdef USE_SRILM
    option.add<std::string>("srilm", 'I', "srilm filename", false,
                            "srilm.arpa");
#endif

    // 解析方式の指定オプション
    option.add<unsigned int>("beam", 'B', "set beam width", false, 5);

    // 出力形式のオプション
    option.add("juman", 'j', "print juman style (default)");
    option.add("morph", 'M', "print morph style");
    option.add<unsigned int>("lattice", 'L', "output lattice format", false, 5);
    option.add("force-single-path", 0,
               "do not output ambiguous words on lattice");

    // 訓練用オプション
    option.add<std::string>("train", 't', "set training data path", false,
                            "data/train.txt");
    option.add<std::string>("outputmodel", 'o', "set model path for output",
                            false, "output.mdl");
    option.add<unsigned int>("iteration", 'i', "iteration number for training",
                             false, 10);
    option.add<double>("Cvalue", 'C', "C value. parameter for SCW", false, 1.0);
    option.add<double>("Phi", 'P', "Phi value. parameter for SCW", false, 1.65);

    option.add<double>(
        "Rweight", 0,
        "linear interpolation parameter for MA score and RNN score", false,
        0.3);
    option.add<double>("Lweight", 0,
                       "linear penalty parameter for unknown words in RNNLM",
                       false, 1.5);

    // デバッグオプション
    option.add("debug", '\0', "debug mode");
    option.add("rnndebug", '\0', "show rnnlm debug message");
    option.add("partial", 0, "receive partially annotated text");
    option.add("static", 0, "static loading for RNNLM. (It may be faster than "
                            "default when you process large texts)");

#ifdef USE_DEV_OPTION
    // 開発用オプション
    option.add<std::string>("gold-lattice", 0,
                            "output gold lattice (- means STDIN)", false,
                            "data/train.txt");
    option.add<unsigned int>("Nmorph", 'N', "print N-best Moprh", false, 5);
    option.add("oldstyle", 0, "print JUMAN style lattice");
    option.add("autoN", 0, "automatically set N depending on sentence length");
    // option.add("typedloss", 0, "use loss function considering form type ");
    option.add("nornnlm", 0, "do not use RNNLM");
    option.add("dynamic", 0,
               "Obsoleted. (It remains only for backward compatibility.)");
    option.add("rnnasfeature", 0, "use rnnlm score as feature (dev)");
    option.add("userep", 0, "use rep in rnnlm (dev)");
    option.add("usebase", 0, "use rep in rnnlm (dev,default)");
    option.add("usesurf", 0, "use surf in rnnlm (dev)");
    option.add("usepos", 0, "use pos in rnnlm (dev,default)");
    option.add("printrep", 0, "print rep(dev)");
#endif

    option.add("version", 'v', "print version");
    option.add("help", 'h', "print this message");
    option.parse_check(argc, argv);
    if (option.exist("version")) { //{{{
        cout << "JUMAN++ " << VERSION << "(" << GITVER << ")" << endl;
        exit(0);
    }                            //}}}
    if (option.exist("train")) { //{{{
        MODE_TRAIN = true;
    } //}}}
} //}}}

#ifndef KKN_UNIT_TEST

int main(int argc, char **argv) { //{{{
    cmdline::parser option;
    std::string data_path = read_jumanpprc();
    option_proc(option, data_path, argc, argv);

    // TODO: オプションの処理，初期化，解析を分離する
    std::string rnnlm_model_path = data_path + "/lang.mdl";
    std::string dict_path = data_path + "/dic";
    std::string model_path = data_path + "/model.mdl";
    std::string freq_word_list = data_path + "/freq_words.list";
    std::string feature_path = data_path + "/feature.def";
    if (option.exist("dir")) {
        rnnlm_model_path = option.get<std::string>("dir") + "/lang.mdl";
        dict_path = option.get<std::string>("dir") + "/dic";
        model_path = option.get<std::string>("dir") + "/model.mdl";
        freq_word_list = option.get<std::string>("dir") + "/freq_words.list";
        feature_path = option.get<std::string>("dir") + "/feature.def";
    }

    if (option.exist("rnnlm"))
        rnnlm_model_path = option.get<std::string>("rnnlm");
    if (option.exist("dict"))
        dict_path = option.get<std::string>("dict");
    if (option.exist("model"))
        model_path = option.get<std::string>("model");
    if (option.exist("use_lexical_feature"))
        freq_word_list = option.get<std::string>("use_lexical_feature");
    if (option.exist("feature"))
        feature_path = option.get<std::string>("feature");

    unsigned int unk_max_length = 2; // 固定

    Morph::Parameter param(dict_path, feature_path,
                           option.get<unsigned int>("iteration"), true,
                           option.exist("train"), unk_max_length,
                           option.exist("debug"), option.exist("lattice"));

    // 訓練時のアウトプット用モデルパスの設定
    param.set_model_filename(option.get<std::string>("outputmodel"));

    param.use_lexical_feature = true;
    param.freq_word_list = freq_word_list;
    Morph::FeatureSet::open_freq_word_set(param.freq_word_list);

    if (param.debug)
        std::cerr << "initializing models ... " << std::flush;

    // ビームサーチを用いるかどうか
    param.set_beam(true);
    param.set_N(5); //初期値

#ifdef USE_DEV_OPTION
    if (option.exist("Nmorph")) {
        param.set_N(option.get<unsigned int>("Nmorph"));
    }
#endif
    if (option.exist("beam")) {
        param.set_N(option.get<unsigned int>("beam"));
    }

    // 学習時のロス関数で 活用型を見る
    param.usetypedloss = true;

    if (option.exist("lattice")) {
        // beam が設定されていたら，lattice のN は表示のみに使う
        if (option.exist("beam")) {
            param.set_L(option.get<unsigned int>("lattice"));
        } else { // 無ければラティスと同じ幅を指定
            // 指定されたビーム幅よりもラティスに使うN-best
            // が多ければ，ビーム幅を拡張する．
            if (option.get<unsigned int>("lattice") > param.N)
                param.set_N(option.get<unsigned int>("lattice"));
            param.set_L(option.get<unsigned int>("lattice"));
        }
    }

    param.set_output_ambigous_word(!option.exist("force-single-path"));

    param.set_use_scw(true);
    param.set_lweight(option.get<double>("Lweight"));
    if (option.exist("Cvalue"))
        param.set_C(option.get<double>("Cvalue"));
    if (option.exist("Phi"))
        param.set_Phi(option.get<double>("Phi"));

    // 部分アノテーション用デリミタ
    param.delimiter = "\t";

#ifdef USE_DEV_OPTION
    if (option.exist("rnnasfeature") && option.exist("rnnlm") &&
        !option.exist("nornnlm")) {
        param.use_rnnlm_as_feature = true;
    }
#endif

    if (option.exist("debug")) {
        Morph::FeatureSet::debug_flag = true;
    }

    // trigram はdefault
    param.set_trigram(true);
    param.set_rweight(option.get<double>("Rweight"));

    param.use_suu_rule = true;
    param.no_posmatch = true;

    // LDA用、廃止
    Morph::Parameter normal_param = param;
    normal_param.set_nbest(true); // nbest を利用するよう設定
    normal_param.set_N(10);       // 10-best に設定

// RNNLM の利用フラグ設定
#ifdef USE_DEV_OPTION
    if (option.exist("userep"))
        param.userep = true;
    if (option.exist("usebase"))
        param.usebase = true;
    if (option.exist("usesurf"))
        param.usesurf = true;
    if (option.exist("usepos"))
        param.usepos = true;

    if (option.exist("nornnlm") &&
        (option.exist("train") && !option.exist("rnnasfeature"))) {
        param.set_rnnlm(false);
        param.set_nce(false);
    } else {
        param.set_rnnlm(true);
        param.set_nce(true);
    }
#else
    if (option.exist("train")) {
        param.set_rnnlm(false);
        param.set_nce(false);
    } else {
        param.set_rnnlm(true);
        param.set_nce(true);
    }
#endif

// SRILM の利用
#ifdef USE_SRILM
    param.set_srilm(option.exist("srilm"));
#endif

    Morph::Tagger tagger(&param);
    Morph::Node::set_param(&param);

    RNNLM::CRnnLM *p_rnnlm;
    if (option.exist("static")) {
        p_rnnlm = new RNNLM::CRnnLM_stat();
    } else {
        p_rnnlm = new RNNLM::CRnnLM_dyn();
    }
    p_rnnlm->setRnnLMFile(rnnlm_model_path.c_str());

    if (option.exist("rnndebug")) {
        p_rnnlm->setDebugMode(1);
        param.rnndebug = true;
    } else {
        p_rnnlm->setDebugMode(0);
    }

    if (param.lpenalty)
        p_rnnlm->setLweight(param.lweight);
    srand(1);
    Morph::Sentence::init_rnnlm_FR(p_rnnlm);

#ifdef USE_SRILM /*{{{*/
    Vocab *vocab;
    Ngram *ngramLM;
    if (option.exist("srilm")) { //
        vocab = new Vocab;
        vocab->unkIsWord() = true; // use unknown <unk>
        // File file(vocabFile, "r"); // vocabFile
        // vocab->read(file); //オプションでは指定してない
        ngramLM = new Ngram(*vocab, 3); // order = 3,or 4?

        std::string lmfile =
            option.get<std::string>("srilm"); // "./srilm.vocab";
        // SRILM::
        File file(lmfile.c_str(), "r"); //
        bool limitVocab = false;        //?

        if (!ngramLM->read(file, limitVocab)) {
            cerr << "format error in lm file " << lmfile << endl;
            exit(1);
        }
        // ngramLM->skipOOVs() = true;
        // ngramLM->linearPenalty() = true; // 未知語のスコアの付け方を変更

        Morph::Sentence::init_srilm(ngramLM, vocab);
    }
#endif /*}}}*/

    if (MODE_TRAIN) { //学習モード{{{
        tagger.train(option.get<std::string>("train"));
        tagger.write_bin_model_file(option.get<std::string>("outputmodel"));
//}}}
#ifdef USE_DEV_OPTION
    } else if (option.exist("gold-lattice")) {
        // GOLD のアノテーション通りに出力するモード
        tagger.read_bin_model_file(model_path);
        param.print_gold = true;
        tagger.output_gold_result(option.get<std::string>("gold-lattice"));
#endif
    } else { // 通常の形態素解析{{{
        tagger.read_bin_model_file(model_path);
        std::ifstream is(argv[1]);

        // sentence loop
        std::string buffer;
        while (getline(is ? is : cin, buffer)) {
            if (buffer.length() < 1) { // empty line
                std::cout << buffer << std::endl;
                continue;
            } else if (buffer.at(0) == '#') {
                if (buffer.length() <= 1) {
                    std::cout << buffer << " JUMAN++:" << VERSION << "("
                              << GITVER << ")" << std::endl;
                    continue;
                }

                // 動的コマンドの処理
                std::size_t pos;
                if ((pos = buffer.find("##JUMAN++\t")) != std::string::npos) {
                    std::size_t arg_pos;
                    // input:
                    // ##JUMAN++<tab>command arg
                    // ##JUMAN++<tab>setL 5

                    std::string command = "set-lattice";
                    if ((arg_pos = buffer.find(command)) != std::string::npos) {
                        arg_pos = buffer.find_first_of(
                            " \t", arg_pos + command.length());
                        long val = std::stol(buffer.substr(arg_pos));
                        param.set_L(val);
                        std::cout << "##JUMAN++\t" << command << " " << val
                                  << std::endl;
                    }

                    command = "set-beam";
                    if ((arg_pos = buffer.find(command)) != std::string::npos) {
                        arg_pos = buffer.find_first_of(
                            " \t", arg_pos + command.length());
                        long val = std::stol(buffer.substr(arg_pos));
                        param.set_N(val);
                        std::cout << "##JUMAN++\t" << command << " " << val
                                  << std::endl;
                    }

                    command = "unset-force-single-path";
                    if ((arg_pos = buffer.find(command)) != std::string::npos) {
                        param.set_output_ambigous_word(true);
                        std::cout << "##JUMAN++\t" << command << " "
                                  << std::endl;
                    }

                    command = "set-force-single-path";
                    if ((arg_pos = buffer.find(command)) != std::string::npos) {
                        param.set_output_ambigous_word(false);
                        std::cout << "##JUMAN++\t" << command << " "
                                  << std::endl;
                    }
                } else { // S-ID の処理
                    std::cout << buffer << " JUMAN++:" << VERSION << "("
                              << GITVER << ")" << std::endl;
                }
                continue;
            }

#if USE_DEV_OPTION
            // N-best の 表示数 自動決定
            if (option.exist("autoN")) {
                Morph::U8string u8buffer(buffer);
                const size_t denom = 10;
                const size_t length = u8buffer.char_size();
                size_t autoN = length / denom;
                if (autoN > option.get<unsigned int>("lattice"))
                    autoN = option.get<unsigned int>("lattice");
                if (autoN < 1)
                    autoN = 1;
                param.set_L(autoN);
            }
#endif

            if (option.exist("partial")) {
                tagger.partial_annotation_analyze(buffer);
            } else {
                tagger.new_sentence_analyze(buffer);
            }

            if (option.exist("lattice")) {
#if USE_DEV_OPTION
                if (option.exist("oldstyle")) {
                    tagger.print_old_lattice();
                } else {
#endif
                    // 通常のNbestラティス
                    tagger.print_lattice_rbeam(param.L);
#if USE_DEV_OPTION
                }
            } else if (option.exist("Nmorph")) {
                // N-best の Moprh形式出力
                tagger.print_beam();
#endif
            } else if (option.exist("morph")) {
#if USE_DEV_OPTION
                if (option.exist("printrep")) {
                    tagger.print_best_beam_rep() else
#endif
                        tagger.print_best_beam(); // Morph出力
                } else {
                    // デフォルトのJUMAN形式の出力
                    tagger.print_best_beam_juman();
                }
                tagger.sentence_clear();
            }
        } //}}}
        return 0;
    } //}}}

#endif

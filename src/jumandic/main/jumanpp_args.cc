//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "jumanpp_args.h"
#ifdef __unix__
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <iostream>
#include <regex>
#include <util/logging.hpp>
#include "args.h"
#include "jpp_rnn_args.h"
#include "util/mmap.h"

namespace jumanpp {
namespace jumandic {

namespace {

struct JppArgsParser {
  args::ArgumentParser parser{"Juman++ v2"};

  args::Positional<std::string> input{parser, "input",
                                      "Input filename (- for stdin)", "-"};

  args::Group general{parser, "General Settings"};
  args::HelpFlag helpFlag{
      general, "HELP", "Prints this message", {'h', "help"}};
  args::ValueFlag<std::string> configFile{
      general, "FILENAME", "Config file location", {'c', "config"}};
  args::ValueFlag<i32> logLevel{general,
                                "LEVEL",
                                "Log level (0 for off, 5 for trace), 0 default",
                                {"log-level"}};
  args::Flag printVersion{
      general, "printVersion", "Just print version and exit", {'v', "version"}};
  args::Flag printDicInfo{
      general, "printModelInfo", "Print model info and exit", {"model-info"}};

  args::Group outputType{parser, "Output type"};
  args::Flag juman{
      outputType, "juman", "Juman style (default)", {'j', "juman"}};
  args::Flag morph{outputType, "morph", "Morph style", {'M', "morph"}};
  args::Flag fullMorph{
      outputType, "full-morph", "Full-morph style", {'F', "full-morph"}};
  args::Flag dicSubset{outputType,
                       "subset",
                       "Subset of dictionary, useful for tests",
                       {"dic-subset"}};
  args::ValueFlag<i32> lattice{outputType,
                               "N",
                               "Lattice style (N-best)",
                               {'L', 's', "lattice", "specifics"}};
  args::Flag segment{outputType, "segment", "Only segment", {"segment"}};
  args::ValueFlag<std::string> segmentSeparator{
      outputType,
      "SEPARATOR",
      "Separator for segmented output (default: space)",
      {"segment-separator"}};
  args::ValueFlag<std::string> graphvis{
      outputType,
      "DIRECTORY",
      "Directory for GraphViz .dot files output",
      {"graphviz-dir"}};

  args::Group modelParams{parser, "Model parameters"};
  args::ValueFlag<std::string> modelFile{
      modelParams, "model", "Model filename", {"model"}};
  args::ValueFlag<std::string> rnnModelFile{
      modelParams, "rnn model", "RNN model filename", {"rnn-model"}};

  args::Group analysisParams{parser, "Analysis parameters"};
  args::ValueFlag<i32> beamSize{
      analysisParams, "N", "Beam size (5 default)", {"beam"}, 5};
  args::ValueFlag<i32> globalBeamSize{
      analysisParams, "N", "Global beam size", {"global-beam"}};
  args::ValueFlag<i32> rightCheckBeam{
      analysisParams, "N", "Right check size", {"right-check"}};
  args::ValueFlag<i32> rightBeamSize{
      analysisParams, "N", "Right beam size", {"right-beam"}};
#ifdef JPP_ENABLE_DEV_TOOLS
  args::Group devParams{parser, "Dev options"};
  args::Flag globalBeamPos{devParams,
                           "globalBeamPos",
                           "Global beam position output",
                           {"global-beam-pos"}};
#if defined(JPP_USE_PROTOBUF)
  args::Flag fullDump{
      outputType, "fullDump", "Full lattice dump (protobuf)", {"full-dump"}};
#endif
#endif

  RnnArgs rnnArgs{parser};

  JppArgsParser() {
    parser.helpParams.gutter = 4;
    parser.helpParams.helpindent = 35;
#if defined(TIOCGWINSZ)
    winsize winsz{0};
    auto sterr_no = fileno(stderr);
    if (ioctl(sterr_no, TIOCGWINSZ, &winsz) == 0 && isatty(sterr_no)) {
      parser.helpParams.width = winsz.ws_col;
      parser.helpParams.useColor = true;
    } else {
      parser.helpParams.width = 10000;
    }
#endif
  }

  Status parseCli(int argc, const char* argv[]) {
    try {
      parser.ParseCLI(argc, argv);
    } catch (args::Help&) {
      std::cerr << parser;
      exit(1);
    } catch (args::ParseError& e) {
      Status::InvalidParameter() << e.what() << "\n" << parser;
    } catch (...) {
      return Status::InvalidParameter() << "unknown exception happened";
    }
    return Status::Ok();
  }

  std::regex separator{"[ \t\n\r]+"};

  Status parseFile(StringPiece filename) {
    util::FullyMappedFile file;
    JPP_RETURN_IF_ERROR(file.open(filename));
    auto data = file.contents();
    auto wstart = data.begin();
    std::regex_token_iterator<const char*> begin{wstart, data.end(), separator};
    std::regex_token_iterator<const char*> end;
    std::vector<std::string> parts;
    for (; begin != end; ++begin) {
      parts.emplace_back(wstart, begin->first);
      wstart = begin->second;
    }
    if (wstart != data.end()) {
      parts.emplace_back(wstart, data.end());
    }
    try {
      parser.ParseArgs(parts);
    } catch (args::Help&) {
      return Status::InvalidParameter() << parser;
    } catch (args::ParseError& e) {
      Status::InvalidParameter() << e.what() << "\n" << parser;
    } catch (...) {
      return Status::InvalidParameter();
    }
    return Status::Ok();
  }

  void fillResult(JumanppConf* result) {
    result->outputType.set(juman, OutputType::Juman);
    result->outputType.set(segment, OutputType::Segmentation);
    result->outputType.set(morph, OutputType::Morph);
    result->outputType.set(fullMorph, OutputType::FullMorph);
    result->outputType.set(dicSubset, OutputType::DicSubset);
    result->outputType.set(lattice, OutputType::Lattice);
    result->beamOutput.set(lattice);
    result->outputType.set(printDicInfo, OutputType::ModelInfo);
    result->outputType.set(printVersion, OutputType::Version);

    result->rnnConfig.mergeWith(rnnArgs.config());

    result->configFile.set(configFile);
    result->logLevel.set(logLevel);
    result->inputFile.set(input);
    result->modelFile.set(modelFile);
    result->rnnModelFile.set(rnnModelFile);
    result->graphvizDir.set(graphvis);
    result->segmentSeparator.set(segmentSeparator);

    result->beamSize.set(beamSize);
    if (result->beamSize < result->beamOutput) {
      result->beamSize = result->beamOutput.value();
    }

    result->globalBeam.set(globalBeamSize);
    result->rightCheck.set(rightCheckBeam);
    result->rightBeam.set(rightBeamSize);

#ifdef JPP_ENABLE_DEV_TOOLS
    result->outputType.set(globalBeamPos, OutputType::GlobalBeamPos);
#if defined(JPP_USE_PROTOBUF)
    result->outputType.set(fullDump, OutputType::FullLatticeDump);
#endif
#endif
  }
};

}  // namespace

Status parseArgs(int argc, const char* argv[], JumanppConf* result) {
  StringPiece myName{"/jumandic.config"};
  JppArgsParser argsParser;
  JumanppConf cmdline;
  JPP_RETURN_IF_ERROR(argsParser.parseCli(argc, argv));
  argsParser.fillResult(&cmdline);

  util::logging::CurrentLogLevel =
      static_cast<util::logging::Level>(cmdline.logLevel.value());

  bool loadedConfig = false;
  Status s = Status::Ok();

  if (!cmdline.configFile.value().empty()) {
    s = argsParser.parseFile(cmdline.configFile.value());
    if (s) {
      argsParser.fillResult(result);
      loadedConfig = true;
    } else {
      return Status::InvalidParameter()
             << "failed to parse provided config at: " << cmdline.configFile
             << "\n"
             << s;
    }
  }

  if (!loadedConfig) {
    std::string globalCfgName{core::JPP_DEFAULT_CONFIG_DIR};
    globalCfgName += myName.str();
    s = argsParser.parseFile(globalCfgName);
    if (s) {
      argsParser.fillResult(result);
      loadedConfig = true;
    }
    LOG_DEBUG() << "tried to read global config from " << globalCfgName
                << " error=" << s;
  }

  if (!loadedConfig) {
    std::string userConfigPath(std::getenv("HOME"));
    userConfigPath += "/.config/jumanpp";
    userConfigPath += myName.str();
    s = argsParser.parseFile(userConfigPath);
    if (s) {
      argsParser.fillResult(result);
    }
    LOG_DEBUG() << "tried to read user config from " << userConfigPath
                << " error=" << s;
  }

  result->mergeWith(cmdline);

  LOG_TRACE() << "Merged Config: " << *result;

  util::logging::CurrentLogLevel =
      static_cast<util::logging::Level>(result->logLevel.value());

  return Status::Ok();
}

}  // namespace jumandic
}  // namespace jumanpp
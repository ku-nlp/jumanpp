//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "jumanpp_args.h"
#include <sys/ioctl.h>
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
  args::Flag printHelp{
      general, "printHelp", "Print this help meassage", {'h', "help"}};
  args::ValueFlag<std::string> configFile{
      general, "FILENAME", "Config file location", {'c', "config"}};
  args::ValueFlag<i32> logLevel{general,
                                "LEVEL",
                                "Log level (0 for off, 5 for trace), 3 default",
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
#endif

  RnnArgs rnnArgs{parser};

  JppArgsParser() {
    parser.helpParams.gutter = 4;
#if defined(TIOCGWINSZ)
    winsize winsz{0};
    if (ioctl(0, TIOCGWINSZ, &winsz) == 0) {
      parser.helpParams.width = std::max<unsigned>(80, winsz.ws_col);
      parser.helpParams.helpindent = 35;
      parser.helpParams.useColor = true;
    }
#endif
  }

  Status parseCli(int argc, const char* argv[]) {
    try {
      parser.ParseCLI(argc, argv);
    } catch (args::Help&) {
      return Status::InvalidParameter() << parser;
    } catch (args::ParseError& e) {
      Status::InvalidParameter() << e.what() << "\n" << parser;
    } catch (...) {
      return Status::InvalidParameter();
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

    result->beamSize.set(beamSize);
    if (result->beamSize < result->beamOutput) {
      result->beamSize = result->beamOutput.value();
    }

    result->globalBeam.set(globalBeamSize);
    result->rightCheck.set(rightCheckBeam);
    result->rightBeam.set(rightBeamSize);

#ifdef JPP_ENABLE_DEV_TOOLS
    result->outputType.set(globalBeamPos, OutputType::GlobalBeamPos);
#endif
    result->outputType.set(printHelp, OutputType::Help);
  }
};

}  // namespace

Status parseArgs(int argc, const char* argv[], JumanppConf* result) {
  StringPiece myName{"/jumandic.config"};
  JppArgsParser argsParser;
  JumanppConf cmdline;
  JPP_RETURN_IF_ERROR(argsParser.parseCli(argc, argv));
  argsParser.fillResult(&cmdline);
  if (cmdline.outputType == OutputType::Help) {
    std::cerr << argsParser.parser;
    std::exit(1);
  }

  util::logging::CurrentLogLevel =
      static_cast<util::logging::Level>(cmdline.logLevel.value());

  std::string globalCfgName{core::JPP_DEFAULT_CONFIG_DIR};
  globalCfgName += myName.str();
  Status s = argsParser.parseFile(globalCfgName);
  if (s) {
    argsParser.fillResult(result);
  }
  LOG_DEBUG() << "tried to read global config from " << globalCfgName
              << " error=" << s;
  std::string userConfigPath(std::getenv("HOME"));
  userConfigPath += "/.config/jumanpp";
  userConfigPath += myName.str();
  s = argsParser.parseFile(userConfigPath);
  if (s) {
    argsParser.fillResult(result);
  }
  LOG_DEBUG() << "tried to read user config from " << userConfigPath
              << " error=" << s;

  if (!cmdline.configFile.value().empty()) {
    s = argsParser.parseFile(cmdline.configFile.value());
    if (s) {
      argsParser.fillResult(result);
    } else {
      return Status::InvalidParameter()
             << "failed to parse provided config at: " << cmdline.configFile
             << "\n"
             << s;
    }
  }

  result->mergeWith(cmdline);

  LOG_TRACE() << "Merged Config: " << *result;

  if (result->modelFile.isDefault()) {
    return Status::InvalidParameter() << "model file was not specified";
  }

  util::logging::CurrentLogLevel =
      static_cast<util::logging::Level>(result->logLevel.value());

  return Status::Ok();
}

}  // namespace jumandic
}  // namespace jumanpp
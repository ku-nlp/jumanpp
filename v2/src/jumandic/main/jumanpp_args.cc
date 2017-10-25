//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "jumanpp_args.h"
#include <sys/ioctl.h>
#include <iostream>
#include "args.h"
#include "jpp_rnn_args.h"

namespace jumanpp {
namespace jumandic {

bool parseArgs(int argc, char* argv[], JumanppConf* result) {
  args::ArgumentParser parser{"Juman++ v2"};

  args::Positional<std::string> input{parser, "input",
                                      "Input filename (- for stdin)", "-"};

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
  args::Flag printVersion{outputType,
                          "printVersion",
                          "Just print version and exit",
                          {'v', "version"}};

  args::Group modelParams{parser, "Model parameters"};
  args::ValueFlag<std::string> modelFile{
      modelParams, "model", "Model filename", {"model"}};
  args::ValueFlag<std::string> rnnModelFile{
      modelParams, "rnn model", "RNN model filename", {"rnn-model"}};

  args::Group analysisParams{parser, "Analysis parameters"};
  args::ValueFlag<i32> beamSize{
      analysisParams, "N", "Beam size (5 default)", {"beam"}, 5};
  args::ValueFlag<i32> globalBeamSize{analysisParams,
                                      "N",
                                      "Global beam size (2*beamSize default)",
                                      {"global-beam"}};
#ifdef JPP_ENABLE_DEV_TOOLS
  args::Group devParams{parser, "Dev options"};
  args::Flag globalBeamPos{devParams,
                           "globalBeamPos",
                           "Global beam position output",
                           {"global-beam-pos"}};
#endif

#if 1
  winsize winsz{0};
  if (ioctl(0, TIOCGWINSZ, &winsz) == 0) {
    parser.helpParams.width = std::max<unsigned>(80, winsz.ws_col);
    parser.helpParams.helpindent = std::max<unsigned>(40, winsz.ws_col / 2);
  }
#endif

  RnnArgs rnnArgs{parser};

  if (result == nullptr) {
    std::cerr << parser;
    exit(1);
  }

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help&) {
    std::cerr << parser;
    exit(1);
  } catch (args::ParseError& e) {
    std::cerr << e.what() << "\n";
    std::cerr << parser;
    exit(1);
  } catch (...) {
    return false;
  }

  result->outputType = OutputType::Juman;
  if (juman) {
    result->outputType = OutputType::Juman;
  }
  if (morph) {
    result->outputType = OutputType::Morph;
  }
  if (fullMorph) {
    result->outputType = OutputType::FullMorph;
  }
  if (dicSubset) {
    result->outputType = OutputType::DicSubset;
  }
  if (lattice) {
    result->outputType = OutputType::Lattice;
    result->beamOutput = lattice.Get();
  }
  if (printVersion) {
    result->printVersion = true;
  }

  result->inputFile = input.Get();

  if (!modelFile) {
    std::cerr << "model file is not specified\n";
    return false;
  }
  result->modelFile = modelFile.Get();
  result->rnnModelFile = rnnModelFile.Get();
  result->rnnConfig = rnnArgs.config();
  result->graphvizDir = graphvis.Get();
  result->beamSize = std::max(beamSize.Get(), result->beamOutput);

  if (globalBeamSize) {
    result->globalBeam = std::max(globalBeamSize.Get(), result->beamSize);
  } else {
    result->globalBeam = result->beamSize * 2;
  }

#ifdef JPP_ENABLE_DEV_TOOLS
  if (globalBeamPos) {
    result->outputType = OutputType::GlobalBeamPos;
  }
#endif

  return true;
}

}  // namespace jumandic
}  // namespace jumanpp
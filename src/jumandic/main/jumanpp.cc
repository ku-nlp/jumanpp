//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "jumanpp.h"
#include <fstream>
#include <iostream>
#include "core/input/pex_stream_reader.h"
#include "jumandic/shared/jumanpp_args.h"
#include "util/logging.hpp"

using namespace jumanpp;

struct InputOutput {
  std::unique_ptr<core::input::StreamReader> streamReader_;
  std::unique_ptr<std::ifstream> fileInput_;
  int currentInFile_ = 0;
  const std::vector<std::string>* inFiles_;
  StringPiece currentInputFilename_;
  std::istream* input_;

  std::unique_ptr<std::ofstream> fileOutput_;
  std::ostream* output_;

  Status moveToNextFile() {
    auto& fn = (*inFiles_)[currentInFile_];
    fileInput_.reset(new std::ifstream{fn});
    if (fileInput_->bad()) {
      return JPPS_INVALID_PARAMETER << "failed to open output file: " << fn;
    }
    input_ = fileInput_.get();
    currentInputFilename_ = fn;
    currentInFile_ += 1;
    return Status::Ok();
  }

  Status nextInput() {
    if (*input_) {
      JPP_RETURN_IF_ERROR(streamReader_->readExample(input_));
      return Status::Ok();
    }

    if (input_->fail()) {
      return JPPS_INVALID_STATE << "failed when reading from file: "
                                << currentInputFilename_;
    }

    return JPPS_NOT_IMPLEMENTED << "should not reach here, it is a bug";
  }

  Status initialize(const jumandic::JumanppConf& conf,
                    const core::CoreHolder& cholder) {
    inFiles_ = &conf.inputFiles.value();
    if (!inFiles_->empty()) {
      JPP_RETURN_IF_ERROR(moveToNextFile());
    } else {
      input_ = &std::cin;
      currentInputFilename_ = "<stdin>";
    }

    if (conf.outputFile == "-") {
      output_ = &std::cout;
    } else {
      fileOutput_.reset(new std::ofstream{conf.outputFile});
      output_ = fileOutput_.get();
    }

    auto inType = conf.inputType.value();
    if (inType == jumandic::InputType::Raw) {
      auto rdr = new core::input::PlainStreamReader{};
      streamReader_.reset(rdr);
      rdr->setMaxSizes(65535, 1024);
    } else {
      auto rdr = new core::input::PexStreamReader{};
      streamReader_.reset(rdr);
      JPP_RETURN_IF_ERROR(rdr->initialize(cholder, '&'));
    }

    return Status::Ok();
  }

  bool hasNext() {
    if (input_->good()) {
      auto ch = input_->peek();
      if (ch == std::char_traits<char>::eof()) {
        return false;
      }
    }
    while (input_->eof() && currentInFile_ < inFiles_->size()) {
      auto s = moveToNextFile();
      if (!s) {
        LOG_ERROR() << s.message();
      }
    }
    auto isOk = !input_->eof();
    return isOk;
  }
};

int main(int argc, const char** argv) {
  std::unique_ptr<std::ifstream> filePtr;

  jumandic::JumanppConf conf;
  Status s = jumandic::parseArgs(argc, argv, &conf);
  if (!s) {
    std::cerr << s << "\n";
    return 1;
  }

  LOG_DEBUG() << "trying to create jumanppexec with model: "
              << conf.modelFile.value()
              << " and rnnmodel=" << conf.rnnModelFile.value();

  jumandic::JumanppExec exec{conf};
  s = exec.init();
  if (!s.isOk()) {
    if (conf.outputType == jumandic::OutputType::Version) {
      exec.printFullVersion();
      return 0;
    }

    if (conf.modelFile.isDefault()) {
      std::cerr << "Model file was not specified\n";
      return 1;
    }

    if (conf.outputType == jumandic::OutputType::ModelInfo) {
      exec.printModelInfo();
      return 0;
    }

    std::cerr << "failed to load model from disk: " << s;
    return 1;
  }

  if (conf.outputType == jumandic::OutputType::Version) {
    exec.printFullVersion();
    return 0;
  }

  if (conf.outputType == jumandic::OutputType::ModelInfo) {
    exec.printModelInfo();
    return 0;
  }

  InputOutput io;

  s = io.initialize(conf, exec.core());
  if (!s) {
    std::cerr << "Failed to initialize I/O: " << s;
    return 1;
  }

  int result = 0;

  while (io.hasNext()) {
    s = io.nextInput();
    if (!s) {
      std::cerr << "failed to read an example: " << s;
      result = 1;
      continue;
    }

    result = 0;

    s = io.streamReader_->analyzeWith(exec.analyzerPtr());
    if (!s) {
      std::cerr << s;
      *io.output_ << exec.emptyResult();
      continue;
    }

    s = exec.format()->format(*exec.analyzerPtr(), io.streamReader_->comment());
    if (!s) {
      std::cerr << s;
    } else {
      *io.output_ << exec.format()->result();
    }
  }

  return result;
}

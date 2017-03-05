//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_TEST_ANALYZER_H
#define JUMANPP_TEST_ANALYZER_H

#include "core/analysis/analyzer_impl.h"
#include "core/core.h"
#include "core/dic_builder.h"
#include "core/impl/model_io.h"
#include "core/runtime_info.h"
#include "core/spec/spec_dsl.h"
#include "core/spec/spec_serialization.h"
#include "testing/standalone_test.h"

namespace jumanpp {
namespace testing {

using namespace jumanpp::core;
using namespace jumanpp::core::analysis;

class TestAnalyzer : public core::analysis::AnalyzerImpl {
  friend class TestEnv;

 public:
  TestAnalyzer(const CoreHolder* core, const AnalyzerConfig& cfg)
      : AnalyzerImpl(core, cfg) {}
  Lattice& lattice() { return lattice_; }
  LatticeBuilder& latticeBuilder() { return latticeBldr_; }
};

class TestEnv {
 public:
  i32 beamSize = 1;
  std::shared_ptr<TestAnalyzer> analyzer;
  std::shared_ptr<CoreHolder> core;
  spec::AnalysisSpec saveLoad;
  AnalyzerConfig cfg;
  dic::DictionaryBuilder origDicBuilder;
  dic::DictionaryBuilder dicBuilder;
  TempFile tmpFile;
  model::FilesystemModel fsModel;
  model::ModelInfo actualInfo;
  RuntimeInfo actualRuntime;
  dic::DictionaryHolder dic;

  template <typename Fn>
  void spec(Fn fn) {
    spec::dsl::ModelSpecBuilder bldr;
    fn(bldr);
    spec::AnalysisSpec original;
    REQUIRE_OK(bldr.build(&original));
    util::CodedBuffer buf;
    spec::saveSpec(original, &buf);
    REQUIRE(spec::loadSpec(buf.contents(), &saveLoad));
  }

  void importDic(StringPiece data, StringPiece name = StringPiece{"test"}) {
    REQUIRE_OK(origDicBuilder.importSpec(&saveLoad));
    REQUIRE_OK(origDicBuilder.importCsv(name, data));
    RuntimeInfo runtimeOrig{};
    dic::DictionaryHolder holder1;
    REQUIRE_OK(holder1.load(origDicBuilder.result()));
    REQUIRE_OK(holder1.compileRuntimeInfo(saveLoad, &runtimeOrig));
    model::ModelInfo nfo{};
    nfo.parts.emplace_back();
    REQUIRE_OK(origDicBuilder.fillModelPart(runtimeOrig, &nfo.parts.back()));
    {
      model::ModelSaver saver;
      REQUIRE_OK(saver.open(tmpFile.name()));
      REQUIRE_OK(saver.save(nfo));
    }
    REQUIRE_OK(fsModel.open(tmpFile.name()));
    REQUIRE_OK(fsModel.load(&actualInfo));
    REQUIRE_OK(dicBuilder.restoreDictionary(actualInfo, &actualRuntime));
    REQUIRE_OK(dic.load(dicBuilder.result()));
    CoreConfig coreConf{beamSize};
    core.reset(new CoreHolder(coreConf, actualRuntime, dic));
    AnalyzerConfig aconf{};
    analyzer.reset(new TestAnalyzer(core.get(), aconf));
  }
};

}  // testing
}  // jumanpp

#endif  // JUMANPP_TEST_ANALYZER_H

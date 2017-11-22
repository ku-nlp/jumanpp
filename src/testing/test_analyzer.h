//
// Created by Arseny Tolmachev on 2017/03/05.
//

#ifndef JUMANPP_TEST_ANALYZER_H
#define JUMANPP_TEST_ANALYZER_H

#include "core/analysis/analysis_result.h"
#include "core/analysis/analyzer_impl.h"
#include "core/core.h"
#include "core/dic/dic_builder.h"
#include "core/env.h"
#include "core/impl/model_io.h"
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
  TestAnalyzer(const CoreHolder* core, const ScoringConfig& sconf,
               const AnalyzerConfig& cfg)
      : AnalyzerImpl(core, sconf, cfg) {}

  LatticeBuilder& latticeBuilder() { return latticeBldr_; }

  Status fullAnalyze(StringPiece input, const ScorerDef* sconf) {
    JPP_RETURN_IF_ERROR(this->resetForInput(input));
    JPP_RETURN_IF_ERROR(this->prepareNodeSeeds());
    JPP_RETURN_IF_ERROR(this->buildLattice());
    JPP_RETURN_IF_ERROR(this->bootstrapAnalysis());
    JPP_RETURN_IF_ERROR(this->computeScores(sconf));
    return Status::Ok();
  }
};

class TestEnv {
 public:
  i32 beamSize = 1;
  std::unique_ptr<TestAnalyzer> analyzer;
  std::unique_ptr<CoreHolder> core;
  spec::AnalysisSpec originalSpec;
  AnalyzerConfig aconf{};
  dic::DictionaryBuilder origDicBuilder;
  dic::BuiltDictionary restoredDic;
  TempFile tmpFile;
  model::FilesystemModel fsModel;
  model::ModelInfo actualInfo;
  dic::DictionaryHolder dic;

  template <typename Fn>
  void spec(Fn fn) {
    spec::dsl::ModelSpecBuilder bldr;
    fn(bldr);
    REQUIRE_OK(bldr.build(&originalSpec));
  }

  void saveDic(const StringPiece& data,
               const StringPiece name = StringPiece{"test"}) {
    REQUIRE_OK(origDicBuilder.importSpec(&originalSpec));
    REQUIRE_OK(origDicBuilder.importCsv(name, data));
    dic::DictionaryHolder holder1;
    REQUIRE_OK(holder1.load(origDicBuilder.result()));
    model::ModelInfo nfo{};
    nfo.parts.emplace_back();
    REQUIRE_OK(origDicBuilder.fillModelPart(&nfo.parts.back()));
    {
      model::ModelSaver saver;
      REQUIRE_OK(saver.open(tmpFile.name()));
      REQUIRE_OK(saver.save(nfo));
    }
  }

  void loadEnv(JumanppEnv* env) { REQUIRE_OK(env->loadModel(tmpFile.name())); }

  void importDic(StringPiece data, StringPiece name = StringPiece{"test"}) {
    saveDic(data, name);
    REQUIRE_OK(fsModel.open(tmpFile.name()));
    REQUIRE_OK(fsModel.load(&actualInfo));
    REQUIRE_OK(restoredDic.restoreDictionary(actualInfo));
    REQUIRE_OK(dic.load(restoredDic));
    core.reset(new CoreHolder(restoredDic.spec, dic));
    REQUIRE_OK(core->initialize(nullptr));
    REQUIRE(core->features().patternDynamic != nullptr);
    REQUIRE(core->features().ngram != nullptr);
    REQUIRE(core->features().ngramPartial != nullptr);
    ScoringConfig scoreConf{beamSize, 1};
    analyzer.reset(new TestAnalyzer(core.get(), scoreConf, aconf));
  }
};

struct ExampleData {
  StringPiece a;
  StringPiece b;
  StringPiece c;

  ExampleData(StringPiece a, StringPiece b, StringPiece c) : a{a}, b{b}, c{c} {}

  bool operator==(const ExampleData& o) const noexcept {
    return a == o.a && b == o.b && c == o.c;
  }

  friend std::ostream& operator<<(std::ostream& os, const ExampleData& data) {
    os << "{a:" << data.a << " b:" << data.b << " c:" << data.c << "}";
    return os;
  }
};

class AnalyzerMethods {
 protected:
  core::analysis::AnalysisPath path;
  NodeWalker walker;
  StringField fa;
  StringField fb;
  StringField fc;
  AnalyzerImpl* analyzer;

 public:
  AnalyzerMethods() : walker{} {}

  explicit AnalyzerMethods(AnalyzerImpl* analyzer) : AnalyzerMethods() {
    REQUIRE_OK(initialize(analyzer));
  }

  Status initialize(AnalyzerImpl* analyzerImpl) {
    analyzer = analyzerImpl;
    auto omg = analyzer->output();
    JPP_RETURN_IF_ERROR(omg.stringField("a", &fa));
    JPP_RETURN_IF_ERROR(omg.stringField("b", &fb));
    JPP_RETURN_IF_ERROR(omg.stringField("c", &fc));
    return Status::Ok();
  }

  ExampleData firstNode(LatticeNodePtr ptr) {
    auto omgr = analyzer->output();
    auto& w = walker;
    REQUIRE(omgr.locate(ptr, &w));
    REQUIRE(w.next());
    return {fa[w], fb[w], fc[w]};
  }

  ExampleData top1Node(int idx) {
    REQUIRE_OK(path.fillIn(analyzer->lattice()));
    path.moveToChunk(idx);
    REQUIRE(path.totalNodesInChunk() == 1);
    ConnectionPtr ptr;
    REQUIRE(path.nextNode(&ptr));
    return firstNode({ptr.boundary, ptr.right});
  }
};

}  // namespace testing
}  // namespace jumanpp

#endif  // JUMANPP_TEST_ANALYZER_H

//
// Created by Arseny Tolmachev on 2017/03/08.
//

#ifndef JUMANPP_TEST_ANALYZER_ENV_H
#define JUMANPP_TEST_ANALYZER_ENV_H

#include <fstream>
#include "core/analysis/analysis_result.h"
#include "core/analysis/perceptron.h"
#include "core/impl/graphviz_format.h"
#include "testing/test_analyzer.h"
#include "util/debug_output.h"
#include "util/logging.hpp"

namespace tests {

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

struct FeatureSet {
  dsl::FieldBuilder& a;
  dsl::FieldBuilder& b;
  dsl::FieldBuilder& c;
  dsl::FeatureBuilder& ph;
};

struct Node {
  std::string surface;
  EntryPtr eptr = EntryPtr::EOS();
  std::vector<i32> entries;
  std::vector<u64> primitve;
  std::vector<u64> pattern;
  std::string f1;
  std::string f2;
};

class PrimFeatureTestEnv {
  TestEnv tenv;
  StringField flda;
  StringField fldb;
  StringListField fldc;

  util::FlatMap<StringPiece, i32> a2idx;

  std::unique_ptr<HashedFeaturePerceptron> hfp;
  ScorerDef sconf;
  AnalysisPath top1;

 public:
  template <typename Fn>
  PrimFeatureTestEnv(StringPiece csvData, Fn fn, i32 beamSize = 1,
                     i32 globalBeam = 0) {
    tenv.beamSize = beamSize;
    tenv.aconf.globalBeamSize = globalBeam;
    tenv.spec([fn](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      auto& b = specBldr.field(2, "b").strings();
      auto& c = specBldr.field(3, "c").stringLists();
      auto& ph = specBldr.feature("ph").placeholder();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::KATAKANA)
          .writeFeatureTo(ph)
          .outputTo({a});
      FeatureSet fs{a, b, c, ph};
      fn(specBldr, fs);
      specBldr.unigram({a, b});
    });
    CHECK(tenv.originalSpec.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
    REQUIRE_OK(tenv.analyzer->output().stringListField("c", &fldc));

    auto sa = tenv.restoredDic.fieldData[0].stringContent;
    dic::impl::StringStorageTraversal sst{sa};
    StringPiece sp;
    while (sst.next(&sp)) {
      a2idx[sp] = sst.position();
    }

    sconf.scoreWeights.push_back(1.0f);
    static float defaultWeights[] = {0.101f, 0.102f, 0.103f, 0.104f};
    hfp.reset(new HashedFeaturePerceptron{defaultWeights});
    sconf.feature = hfp.get();
    REQUIRE_OK(tenv.analyzer->initScorers(sconf));
  }

  AnalysisPath& top() { return top1; }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->prepareNodeSeeds());
    CHECK_OK(tenv.analyzer->buildLattice());
  }

  void analyze2(StringPiece str) {
    analyze(str);
    REQUIRE_OK(tenv.analyzer->bootstrapAnalysis());
    CHECK_OK(tenv.analyzer->computeScores(&sconf));
    top1.fillIn(tenv.analyzer->lattice());
  }

  size_t numNodeSeeds() const {
    return tenv.analyzer->latticeBuilder().seeds().size();
  }

  Node makeNode(EntryPtr eptr, LatticeNodePtr lnp) {
    auto om = this->tenv.analyzer->output();
    auto w = om.nodeWalker();
    REQUIRE(om.locate(eptr, &w));
    REQUIRE(w.next());
    Node n;
    n.eptr = eptr;
    n.surface = flda[w].str();
    n.f1 = flda[w].str();
    n.f2 = fldb[w].str();
    util::copy_insert(w.features(), n.primitve);
    auto bnd = tenv.analyzer->lattice()->boundary(lnp.boundary);
    auto patterns = bnd->starts()->patternFeatureData().row(lnp.position);
    util::copy_insert(patterns, n.pattern);

    return n;
  }

  Node uniqueNode(StringPiece surface, i32 position) {
    CAPTURE(surface);
    CAPTURE(position);
    auto& l = *tenv.analyzer->lattice();
    auto bndIdx = position + 2;
    auto b = l.boundary(bndIdx);
    auto starts = b->starts();
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    EntryPtr x = EntryPtr::EOS();
    i32 pos = 0;
    i32 i = 0;
    for (auto& ninfo : starts->nodeInfo()) {
      REQUIRE(output.locate(ninfo.entryPtr(), &walker));
      REQUIRE(walker.next());
      auto cont = flda[walker];
      if (cont == surface) {
        if (x != EntryPtr::EOS()) {
          FAIL("found second node with surface " << surface);
        }
        x = ninfo.entryPtr();
        pos = i;
      }
      ++i;
    }
    if (x == EntryPtr::EOS()) {
      FAIL("could not find a node");
    }

    return makeNode(x, LatticeNodePtr::make(bndIdx, pos));
  }

  const OutputManager& output() const { return tenv.analyzer->output(); }

  Node uniqueNode(StringPiece af, StringPiece bf, i32 position) {
    CAPTURE(af);
    CAPTURE(bf);
    CAPTURE(position);
    auto& l = *tenv.analyzer->lattice();
    auto b = l.boundary(position + 2);
    auto starts = b->starts();
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    EntryPtr x = EntryPtr::EOS();
    i32 pos = 0;
    i32 i = 0;
    for (auto& ninfo : starts->nodeInfo()) {
      REQUIRE(output.locate(ninfo.entryPtr(), &walker));
      REQUIRE(walker.next());
      auto cont = flda[walker];
      auto nodefb = fldb[walker];
      if (cont == af && bf == nodefb) {
        if (x != EntryPtr::EOS()) {
          FAIL("found second node with surface " << af);
        }
        x = ninfo.entryPtr();
        pos = i;
      }
      ++i;
    }
    if (x == EntryPtr::EOS()) {
      FAIL("could not find a node " << position << ": [" << af << " " << bf
                                    << "]");
    }

    return makeNode(x, LatticeNodePtr::make(position + 2, pos));
  }

  i32 idxa(StringPiece data) {
    auto it = a2idx.find(data);
    if (it == a2idx.end()) {
      return -5000;
    }
    return it->second;
  }

  const spec::AnalysisSpec& spec() const { return tenv.restoredDic.spec; }

  void printAll() {
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    auto seeds = tenv.analyzer->latticeBuilder().seeds();

    for (auto& seed : seeds) {
      CHECK(output.locate(seed.entryPtr, &walker));
      while (walker.next()) {
        WARN("NODE [" << seed.codepointStart << ", " << seed.codepointEnd
                      << "]: " << flda[walker] << " " << fldb[walker]);
      }
    }
  }

  Node target(const ConnectionPtr& ptr) {
    auto bnd = tenv.analyzer->lattice()->boundary(ptr.boundary);
    auto starts = bnd->starts();
    Node n;
    auto pos = ptr.right;
    util::copy_insert(starts->entryData().row(pos), n.entries);
    util::copy_insert(starts->patternFeatureData(), n.pattern);
    n.eptr = starts->nodeInfo().at(pos).entryPtr();

    auto o = tenv.analyzer->output();
    auto w = o.nodeWalker();
    REQUIRE(o.locate(LatticeNodePtr{ptr.boundary, ptr.right}, &w));
    n.f1 = flda[w].str();
    n.f2 = fldb[w].str();
    return n;
  }

  void dumpTrainers(StringPiece baseDir, int cnt = 0) {
    auto bldr = core::format::GraphVizBuilder{};
    bldr.row({"a", "b"});
    core::format::GraphVizFormat fmt;
    REQUIRE_OK(bldr.build(&fmt, tenv.beamSize));

    char buffer[256];

    std::snprintf(buffer, 255, "%s/dump_%d.dot", baseDir.char_begin(), cnt);
    std::ofstream out{buffer};
    auto pana = tenv.analyzer.get();
    REQUIRE_OK(fmt.initialize(pana->output()));
    fmt.reset();
    auto lat = pana->lattice();
    REQUIRE_OK(fmt.render(lat));
    out << fmt.result();
  }

  ~PrimFeatureTestEnv() {
    if (!Catch::getResultCapture().lastAssertionPassed()) {
      printAll();
    }
  }
};
}  // namespace tests

#endif  // JUMANPP_TEST_ANALYZER_ENV_H

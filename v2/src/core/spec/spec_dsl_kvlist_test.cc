//
// Created by Arseny Tolmachev on 2017/10/03.
//

#include <fstream>
#include "core/analysis/analysis_result.h"
#include "core/analysis/perceptron.h"
#include "core/impl/graphviz_format.h"
#include "core/spec/spec_dsl.h"
#include "testing/test_analyzer.h"

using namespace jumanpp::core::spec::dsl;
using namespace jumanpp::core::spec;
using jumanpp::chars::CharacterClass;

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

namespace {

struct KVPair {
  std::string key;
  std::string value;
  bool hasValue;
};

struct Features {
  FieldBuilder& a;
  FieldBuilder& b;
  FieldBuilder& c;
  FeatureBuilder& ph;
};

struct Node {
  std::string surface;
  EntryPtr eptr = EntryPtr::EOS();
  std::vector<i32> entries;
  std::vector<u64> primitve;
  std::vector<u64> pattern;
  std::string f1;
  std::string f2;
  std::vector<KVPair> f3;
};

class KVListTestEnv {
  TestEnv tenv;
  StringField flda;
  StringField fldb;
  KVListField fldc;

  util::FlatMap<StringPiece, i32> a2idx;

  std::unique_ptr<HashedFeaturePerceptron> hfp;
  ScorerDef sconf;
  AnalysisPath top1;

 public:
  template <typename Fn>
  KVListTestEnv(StringPiece csvData, Fn fn, i32 beamSize = 1) {
    tenv.beamSize = beamSize;
    tenv.spec([fn](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      auto& b = specBldr.field(2, "b").strings();
      auto& c = specBldr.field(3, "c").kvLists();
      auto& ph = specBldr.feature("ph").placeholder();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::KATAKANA)
          .notPrefixOfDicFeature(ph)
          .outputTo({a});
      auto f = Features{a, b, c, ph};
      fn(specBldr, f);
      specBldr.unigram({a, b});
    });
    CHECK(tenv.saveLoad.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
    REQUIRE_OK(tenv.analyzer->output().kvListField("c", &fldc));

    auto sa = tenv.dicBuilder.result().fieldData[0].stringContent;
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

  Node uniqueNode(StringPiece surface, i32 position) {
    CAPTURE(surface);
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

    Node n;
    fillNode(n, LatticeNodePtr{static_cast<u16>(position + 2),
                               static_cast<u16>(pos)});
    return n;
  }

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

    Node n;
    fillNode(n, LatticeNodePtr{static_cast<u16>(position + 2),
                               static_cast<u16>(pos)});
    return n;
  }

  i32 idxa(StringPiece data) {
    auto it = a2idx.find(data);
    if (it == a2idx.end()) {
      return -5000;
    }
    return it->second;
  }

  util::ArraySlice<u64> prim(i32 start, i32 node) {
    CAPTURE(start);
    auto bnd = tenv.analyzer->lattice()->boundary(start + 2);
    auto pBoundary = bnd->starts();
    REQUIRE(pBoundary->arraySize() > 0);
    return pBoundary->primitiveFeatureData().row(node);
  }

  const spec::AnalysisSpec& spec() const { return tenv.dicBuilder.spec(); }

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

  void fillNode(Node& n, LatticeNodePtr ptr) {
    auto bnd = tenv.analyzer->lattice()->boundary(ptr.boundary);
    auto starts = bnd->starts();
    auto pos = ptr.position;
    util::copy_insert(starts->entryData().row(pos), n.entries);
    util::copy_insert(starts->primitiveFeatureData(), n.primitve);
    util::copy_insert(starts->patternFeatureData(), n.pattern);
    n.eptr = starts->nodeInfo().at(pos).entryPtr();

    auto o = tenv.analyzer->output();
    auto w = o.nodeWalker();
    REQUIRE(o.locate(ptr, &w));
    n.f1 = flda[w].str();
    n.f2 = fldb[w].str();
    n.surface = n.f1;

    auto kviter = fldc[w];
    while (kviter.next()) {
      KVPair kvp;
      kvp.key = kviter.key().str();
      if (kviter.hasValue()) {
        kvp.hasValue = true;
        kvp.value = kviter.value().str();
      } else {
        kvp.hasValue = false;
      }
      n.f3.emplace_back(kvp);
    }
  }

  Node target(const ConnectionPtr& ptr) {
    Node n;
    fillNode(n, ptr.latticeNodePtr());
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

  ~KVListTestEnv() {
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      printAll();
    }
  }
};

}  // namespace

TEST_CASE("can create a spec with kvlist") {
  ModelSpecBuilder bldr;
  bldr.field(1, "a").strings().trieIndex();
  bldr.field(2, "b").kvLists();
  AnalysisSpec spec;
  CHECK_OK(bldr.build(&spec));
}

TEST_CASE("can build a small dic with kvlist") {
  StringPiece data = "z,z,\na,x,0\nb,y,0:2\n";
  KVListTestEnv env{data, [](ModelSpecBuilder& b, Features f) {}};
  env.analyze("aba");
  auto n0 = env.uniqueNode("a", 0);
  CHECK(n0.f3.size() == 1);
  CHECK(n0.f3[0].key == "0");
  CHECK_FALSE(n0.f3[0].hasValue);
  auto n1 = env.uniqueNode("b", 1);
  CHECK(n1.f3.size() == 1);
  CHECK(n1.f3[0].key == "0");
  CHECK(n1.f3[0].hasValue);
  CHECK(n1.f3[0].value == "2");
}

TEST_CASE("can build a small dic with kvlist with multiple elems") {
  StringPiece data = "z,z,\na,x,0 3\nb,y,0:2 1:2 3:7 6:0\n";
  KVListTestEnv env{data, [](ModelSpecBuilder& b, Features f) {}};
  env.analyze("aba");
  auto n0 = env.uniqueNode("a", 0);
  CHECK(n0.f3.size() == 2);
  CHECK(n0.f3[0].key == "0");
  CHECK(n0.f3[1].key == "3");
  CHECK_FALSE(n0.f3[0].hasValue);
  auto n1 = env.uniqueNode("b", 1);
  CHECK(n1.f3.size() == 4);
  CHECK(n1.f3[0].key == "0");
  CHECK(n1.f3[0].hasValue);
  CHECK(n1.f3[0].value == "2");
  CHECK(n1.f3[1].key == "3");
  CHECK(n1.f3[1].hasValue);
  CHECK(n1.f3[1].value == "7");
}

TEST_CASE("identical kvlists have identical pointers") {
  StringPiece data = "z,z,\na,x,0:2\nb,y,0:1\nc,z,0:2\n";
  KVListTestEnv env{data, [](ModelSpecBuilder& b, Features f) {}};
  env.analyze("abc");
  auto n0 = env.uniqueNode("a", 0);
  auto n1 = env.uniqueNode("b", 1);
  auto n2 = env.uniqueNode("c", 2);
  CHECK(n0.entries[2] == n2.entries[2]);
  CHECK(n0.entries[2] != n1.entries[2]);
}

TEST_CASE("identical kvlist works with matchValue feature (key)") {
  StringPiece data = "z,z,\na,x,0:2\nb,y,1:0\nc,z,0:1\n";
  KVListTestEnv env{data, [](ModelSpecBuilder& b, Features f) {
                      auto& f2 = b.feature("l0").matchData(f.c, "0");
                      b.unigram({f2});
                    }};
  env.analyze2("abc");
  auto n0 = env.uniqueNode("a", 0);
  auto n1 = env.uniqueNode("b", 1);
  auto n2 = env.uniqueNode("c", 2);
  CHECK(n0.primitve[3] == 1);
  CHECK(n1.primitve[3] == 0);
  CHECK(n2.primitve[3] == 1);
}
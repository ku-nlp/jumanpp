//
// Created by Arseny Tolmachev on 2017/03/06.
//

#include "testing/test_analyzer.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;
using namespace jumanpp::testing;
using namespace jumanpp;

namespace {

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
};

class PrimFeatureTestEnv {
  TestEnv tenv;
  StringField flda;
  StringField fldb;
  StringListField fldc;

  util::FlatMap<StringPiece, i32> a2idx;

 public:
  template <typename Fn>
  PrimFeatureTestEnv(StringPiece csvData, Fn fn) {
    tenv.spec([fn](dsl::ModelSpecBuilder& specBldr) {
      auto& a = specBldr.field(1, "a").strings().trieIndex();
      auto& b = specBldr.field(2, "b").strings();
      auto& c = specBldr.field(3, "c").stringLists();
      auto& ph = specBldr.feature("ph").placeholder();
      specBldr.unk("chars", 1)
          .chunking(chars::CharacterClass::KATAKANA)
          .notPrefixOfDicFeature(ph)
          .outputTo({a});
      FeatureSet fs{a, b, c, ph};
      fn(specBldr, fs);
      specBldr.unigram({a, b});
    });
    CHECK(tenv.saveLoad.unkCreators.size() == 1);
    tenv.importDic(csvData);
    REQUIRE_OK(tenv.analyzer->output().stringField("a", &flda));
    REQUIRE_OK(tenv.analyzer->output().stringField("b", &fldb));
    REQUIRE_OK(tenv.analyzer->output().stringListField("c", &fldc));

    auto sa = tenv.dicBuilder.result().fieldData[0].stringContent;
    dic::impl::StringStorageTraversal sst{sa};
    StringPiece sp;
    while (sst.next(&sp)) {
      a2idx[sp] = sst.position();
    }
  }

  void analyze(StringPiece str) {
    CAPTURE(str);
    CHECK_OK(tenv.analyzer->resetForInput(str));
    CHECK_OK(tenv.analyzer->makeNodeSeedsFromDic());
    CHECK_OK(tenv.analyzer->buildLattice());
  }

  size_t numNodeSeeds() const {
    return tenv.analyzer->latticeBuilder().seeds().size();
  }

  Node uniqueNode(StringPiece surface, i32 position) {
    CAPTURE(surface);
    CAPTURE(position);
    auto& l = tenv.analyzer->lattice();
    auto b = l.boundary(position + 2);
    auto starts = b->starts();
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    EntryPtr x = EntryPtr::EOS();
    i32 pos = 0;
    i32 i = 0;
    for (auto& eptr : starts->entryPtrData()) {
      REQUIRE(output.locate(eptr, &walker));
      auto cont = flda[walker];
      if (cont == surface) {
        if (x != EntryPtr::EOS()) {
          FAIL("found second node with surface " << surface);
        }
        x = eptr;
        pos = i;
      }
      ++i;
    }
    if (x == EntryPtr::EOS()) {
      FAIL("could not find a node");
    }

    Node n;
    n.surface = surface.str();
    n.eptr = x;
    util::copy_insert(starts->entryData().row(pos), n.entries);
    util::copy_insert(starts->primitiveFeatureData().row(pos), n.primitve);
    util::copy_insert(starts->patternFeatureData().row(pos), n.pattern);
    return n;
  }

  Node uniqueNode(StringPiece af, StringPiece bf, i32 position) {
    CAPTURE(af);
    CAPTURE(bf);
    CAPTURE(position);
    auto& l = tenv.analyzer->lattice();
    auto b = l.boundary(position + 2);
    auto starts = b->starts();
    auto& output = tenv.analyzer->output();
    auto walker = output.nodeWalker();
    EntryPtr x = EntryPtr::EOS();
    i32 pos = 0;
    i32 i = 0;
    for (auto& eptr : starts->entryPtrData()) {
      REQUIRE(output.locate(eptr, &walker));
      auto cont = flda[walker];
      auto nodefb = fldb[walker];
      if (cont == af && bf == nodefb) {
        if (x != EntryPtr::EOS()) {
          FAIL("found second node with surface " << af);
        }
        x = eptr;
        pos = i;
      }
      ++i;
    }
    if (x == EntryPtr::EOS()) {
      FAIL("could not find a node " << position << ": [" << af << " " << bf
                                    << "]");
    }

    Node n;
    n.surface = af.str();
    n.eptr = x;
    util::copy_insert(starts->entryData().row(pos), n.entries);
    util::copy_insert(starts->primitiveFeatureData().row(pos), n.primitve);
    util::copy_insert(starts->patternFeatureData().row(pos), n.pattern);
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
    auto bnd = tenv.analyzer->lattice().boundary(start + 2);
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

  ~PrimFeatureTestEnv() {
    if (!Catch::getResultCapture().getLastResult()->isOk()) {
      printAll();
    }
  }
};
}

TEST_CASE("primitive features are computed") {
  StringPiece dic = "XXX,z,KANA\na,b,\nb,c,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}};
  env.analyze("ab");
  auto p = env.uniqueNode("a", 0);
  CHECK(p.primitve[0] == env.idxa("a"));
  auto p2 = env.prim(1, 0);
  CHECK(p2[0] == env.idxa("b"));
  CHECK(p.primitve[0] != p2[0]);
}

TEST_CASE("not prefix feature works") {
  StringPiece dic = "XXX,z,KANA\nカラ,b,\nb,c,\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {}};
  env.analyze("カラフ");
  auto p1 = env.uniqueNode("カラ", 0);
  CHECK(p1.eptr.isDic());
  CHECK(p1.primitve[0] == env.idxa("カラ"));
  CHECK(p1.primitve.size() == 3);
  CHECK(p1.primitve[2] == 0);
  auto p2 = env.uniqueNode("カ", 0);
  CHECK(p2.eptr.isSpecial());
  CHECK(p2.entries[2] != 0);
  CHECK(p2.primitve[2] == 0);
  auto p3 = env.uniqueNode("カラフ", 0);
  CHECK(p3.eptr.isSpecial());
  CHECK(p3.primitve[2] == 1);
}

TEST_CASE("match string feature works") {
  StringPiece dic = "XXX,z,KANA\nカラ,b,\nb,c,\n";
  PrimFeatureTestEnv env{dic,
                         [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
                           specBldr.feature("mtch").matchValue(fs.b, "b");
                         }};
  REQUIRE(env.spec().features.primitive[3].name == "mtch");
  env.analyze("カラフ");
  auto p1 = env.uniqueNode("カラ", 0);
  CHECK(p1.primitve.size() == 4);
  CHECK(p1.primitve[3] == 1);
  auto p2 = env.uniqueNode("カ", 0);
  CHECK(p2.primitve.size() == 4);
  CHECK(p2.primitve[3] == 0);
}

TEST_CASE("match list feature works") {
  StringPiece dic = "XXX,z,KANA\nカラ,b,\nb,c,\n";
  PrimFeatureTestEnv env{dic,
                         [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
                           specBldr.feature("mtch").matchValue(fs.c, "KANA");
                         }};
  REQUIRE(env.spec().features.primitive[3].name == "mtch");
  env.analyze("カラフ");
  auto p1 = env.uniqueNode("カラ", 0);
  CHECK(p1.primitve.size() == 4);
  CHECK(p1.primitve[3] == 0);
  auto p2 = env.uniqueNode("カ", 0);
  CHECK(p2.primitve.size() == 4);
  CHECK(p2.primitve[3] == 1);
}

TEST_CASE("match list feature works with several entries") {
  StringPiece dic = "XXX,z,XANA DATA\nカラ,b,XAS KANA BAR\nラ,c,A B KANA\n";
  PrimFeatureTestEnv env{dic,
                         [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
                           specBldr.feature("mtch").matchValue(fs.c, "KANA");
                         }};
  REQUIRE(env.spec().features.primitive[3].name == "mtch");
  env.analyze("カラフ");
  auto p1 = env.uniqueNode("カラ", 0);
  CHECK(p1.primitve.size() == 4);
  CHECK(p1.primitve[3] == 1);
  auto p2 = env.uniqueNode("カ", 0);
  CHECK(p2.primitve.size() == 4);
  CHECK(p2.primitve[3] == 0);
  auto p3 = env.uniqueNode("ラ", 1);
  CHECK(p3.primitve.size() == 4);
  CHECK(p3.primitve[3] == 1);
  auto p4 = env.uniqueNode("ラフ", 1);
  CHECK(p4.primitve.size() == 4);
  CHECK(p4.primitve[3] == 0);
}

TEST_CASE("match csv feature works with single target") {
  StringPiece dic =
      "XXX,z,x\n"
      "a,a,xR\n"
      "sa,a,xR\n"
      "ab,b,xR\n"
      "ab,d,xR\n"
      "sab,c,xR\n"
      "aab,d,xR\n"
      "b,e,xR\n"
      "ar,b,xR\n"
      "bar,e,x\n";
  PrimFeatureTestEnv env{dic,
                         [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
                           specBldr.feature("mtch")
                               .matchAnyRowOfCsv("z\nd\nf\ng", {fs.b})
                               .ifTrue({fs.a, fs.b})
                               .ifFalse({fs.b});
                         }};
  env.analyze("saabar");
  auto n1 = env.uniqueNode("ab", "b", 2);
  auto n2 = env.uniqueNode("ab", "d", 2);
  auto n3 = env.uniqueNode("ar", "b", 4);
  CHECK(n1.primitve.size() == 4);
  CHECK(n1.primitve[3] == n3.primitve[3]);
  CHECK(n1.primitve[3] != n2.primitve[3]);
  CHECK(n1.primitve[3] != n2.primitve[3]);
}

TEST_CASE("match csv feature works with complex target") {
  StringPiece dic =
      "XXX,z,x\n"
      "a,a,xR\n"
      "sa,a,xR\n"
      "ab,b,xR\n"
      "ab,d,xR\n"
      "saa,b,xR\n"
      "sab,c,xR\n"
      "aab,d,xR\n"
      "b,e,xR\n"
      "ar,b,xR\n"
      "bar,e,x\n";
  PrimFeatureTestEnv env{
      dic, [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
        specBldr.feature("mtch")
            .matchAnyRowOfCsv("x,z\nab,d\nb,e\nar,b", {fs.a, fs.b})
            .ifTrue({fs.a, fs.b})
            .ifFalse({fs.b});
      }};
  env.analyze("saabar");
  auto n1 = env.uniqueNode("ab", "b", 2);
  auto n2 = env.uniqueNode("ab", "d", 2);
  auto n3 = env.uniqueNode("ar", "b", 4);
  auto n4 = env.uniqueNode("saa", "b", 0);
  CHECK(n1.primitve.size() == 4);
  CHECK(n1.primitve[3] == n4.primitve[3]);
  CHECK(n1.primitve[3] != n3.primitve[3]);
  CHECK(n1.primitve[3] != n2.primitve[3]);
  CHECK(n1.primitve[3] != n2.primitve[3]);
}

TEST_CASE("same values from different columns have different hash") {
  StringPiece dic =
      "z,z,z\n"
      "a,a,a\n"
      "b,b,b\n"
      "c,c,c\n"
      "d,d,d\n"
      "e,e,e\n"
      "f,f,f\n";
  PrimFeatureTestEnv env{dic,
                         [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
                           specBldr.unigram({fs.a});
                           specBldr.unigram({fs.b});
                         }};
  env.analyze("bad");
  auto n1 = env.uniqueNode("a", 1);
  auto n2 = env.uniqueNode("b", 0);
  auto n3 = env.uniqueNode("d", 2);

  // pattern 0 is default
  CHECK(n1.pattern.size() == 3);
  CHECK(n1.primitve[0] == n1.primitve[1]);
  CHECK(n1.pattern[1] != n1.pattern[2]);
  CHECK(n2.primitve[0] == n2.primitve[1]);
  CHECK(n2.pattern[1] != n2.pattern[2]);
  CHECK(n3.primitve[0] == n3.primitve[1]);
  CHECK(n3.pattern[1] != n3.pattern[2]);
}

TEST_CASE("same values from same columns have same hash") {
  StringPiece dic =
      "z,z,z\n"
      "a,a,a\n"
      "b,b,b\n"
      "c,c,c\n"
      "d,a,d\n";
  PrimFeatureTestEnv env{dic,
                         [](dsl::ModelSpecBuilder& specBldr, FeatureSet& fs) {
                           specBldr.unigram({fs.a});
                           specBldr.unigram({fs.b});
                         }};
  env.analyze("bad");
  auto n1 = env.uniqueNode("a", 1);
  auto n2 = env.uniqueNode("d", 2);

  // pattern 0 is default
  CHECK(n1.pattern.size() == 3);
  CHECK(n1.pattern[2] != n2.pattern[2]);
  CHECK(n1.pattern[3] != n2.pattern[3]);
}
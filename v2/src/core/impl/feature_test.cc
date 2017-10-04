//
// Created by Arseny Tolmachev on 2017/03/06.
//

#include "core/test/test_analyzer_env.h"

using namespace tests;

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
                           specBldr.feature("mtch").matchData(fs.b, "b");
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
                           specBldr.feature("mtch").matchData(fs.c, "KANA");
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
                           specBldr.feature("mtch").matchData(fs.c, "KANA");
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

  CHECK(n1.pattern.size() == 3);
  CHECK(n1.primitve[0] == n1.primitve[1]);
  CHECK(n1.pattern[0] != n1.pattern[1]);
  CHECK(n2.primitve[0] == n2.primitve[1]);
  CHECK(n2.pattern[0] != n2.pattern[1]);
  CHECK(n3.primitve[0] == n3.primitve[1]);
  CHECK(n3.pattern[0] != n3.pattern[1]);
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

  CHECK(n1.pattern.size() == 3);
  CHECK(n1.primitve[0] != n2.primitve[0]);
  CHECK(n1.pattern[0] != n2.pattern[0]);
  CHECK(n1.primitve[1] == n2.primitve[1]);
  CHECK(n1.pattern[1] == n2.pattern[1]);
}
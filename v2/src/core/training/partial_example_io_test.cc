//
// Created by Arseny Tolmachev on 2017/10/11.
//

#include "partial_example.h"
#include "training_test_common.h"

namespace {

class GoldExample2Env {
 protected:
  testing::TestEnv env;
  AnalyzerMethods<4> am;

 public:
  GoldExample2Env(StringPiece dic, bool katakanaUnks = false) {
    env.beamSize = 3;
    env.spec([katakanaUnks](core::spec::dsl::ModelSpecBuilder& bldr) {
      auto& a = bldr.field(1, "a").strings().trieIndex();
      auto& b = bldr.field(2, "d").strings();
      auto& c = bldr.field(3, "b").strings().stringStorage(b);
      auto& d = bldr.field(4, "c").strings().stringStorage(b);
      bldr.unigram({a, b});
      bldr.bigram({a}, {a});
      bldr.bigram({b}, {b});
      bldr.bigram({a, b}, {a, b});
      bldr.bigram({a, d}, {a, d});
      bldr.trigram({a}, {a}, {a});
      bldr.train().field(d, 1.0f).field(b, 1.0f).field(c, 0.0f).field(a, 1.0f);

      if (katakanaUnks) {
        bldr.unk("katakana", 1)
            .chunking(chars::CharacterClass::KATAKANA)
            .outputTo({a});
      }
    });
    env.importDic(dic);
    ScorerDef scorers{};
    scorers.scoreWeights.push_back(1.0f);
    REQUIRE_OK(env.analyzer->initScorers(scorers));
    am.initialize(env.analyzer.get());
  }

  const core::spec::AnalysisSpec& spec() const { return env.originalSpec; }

  testing::TestAnalyzer* anaImpl() { return env.analyzer.get(); }

  const core::CoreHolder& core() const { return *env.core; }

  ExampleData firstNode(LatticeNodePtr ptr) { return am.firstNode(ptr); }

  ExampleData top1Node(int idx) { return am.top1Node(idx); }
};

}  // namespace

namespace t = jumanpp::core::training;
namespace a = jumanpp::core::analysis;

TEST_CASE("can read partitial example") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "# test\nもも\nも\nもも\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "ももももも");
  CHECK(pe.exampleInfo().line == 1);
  CHECK(pe.exampleInfo().comment == "test");
  CHECK(pe.exampleInfo().file == "<memory>");
  CHECK(pe.nodes().empty());
  CHECK(pe.boundaries().size() == 2);
  CHECK(pe.boundaries()[0] == 4);
  CHECK(pe.boundaries()[1] == 5);
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}

TEST_CASE("can read empty example") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.nodes().empty());
  CHECK(pe.surface().empty());
  CHECK(pe.boundaries().empty());
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}

TEST_CASE("can read example with a single element") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "もも\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "もも");
  CHECK(pe.nodes().empty());
  CHECK(pe.boundaries().empty());
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}

TEST_CASE("can read example with a single tagged element (0 tags)") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "\tもも\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "もも");
  CHECK(pe.nodes().size() == 1);
  CHECK(pe.nodes()[0].tags.empty());
  CHECK(pe.boundaries().empty());
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}

TEST_CASE("can read example with a single tagged element (1 tags)") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\nL,2,b,B\n";
  StringPiece ex = "\tもも\td:A\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "もも");
  CHECK(pe.nodes().size() == 1);
  CHECK(pe.boundaries().empty());
  auto& tags = pe.nodes()[0].tags;
  CHECK(tags.size() == 1);
  CHECK(tags[0].field == 1);
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}

TEST_CASE("can read example with a single tagged element (3 tags)") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\nL,2,b,B\n";
  StringPiece ex = "\tもも\tb:b\td:1\tc:B\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "もも");
  CHECK(pe.nodes().size() == 1);
  CHECK(pe.boundaries().empty());
  auto& tags = pe.nodes()[0].tags;
  CHECK(tags.size() == 3);
  CHECK(tags[0].field == 2);
  CHECK(tags[1].field == 1);
  CHECK(tags[2].field == 3);
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}

TEST_CASE("can read tagged and single example") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "も\n\tもも\n\n\tも\nも\n\tも\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "ももも");
  CHECK(pe.nodes().size() == 1);
  CHECK(pe.nodes()[0].tags.empty());
  CHECK(pe.boundaries().size() == 1);
  CHECK(pe.boundaries()[0] == 3);
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(pe2.surface() == "ももも");
  CHECK(pe2.nodes().size() == 2);
  CHECK(pe2.nodes()[0].tags.empty());
  CHECK(pe2.nodes()[1].tags.empty());
  CHECK(pe2.boundaries().size() == 2);
  CHECK(pe2.boundaries()[0] == 3);
  CHECK(pe2.boundaries()[1] == 4);
  CHECK_FALSE(eof);
  t::PartialExample pe3;
  REQUIRE_OK(rdr.readExample(&pe3, &eof));
  CHECK(eof);
}

TEST_CASE("can read two examples to the same place") {
  StringPiece dic = "X,X,Y,Z\nもも,1,a,A\nも,2,b,B\n";
  StringPiece ex = "も\n\tもも\n\n\tも\nも\n\tも\n\n";
  GoldExample2Env env{dic};
  t::TrainingIo tio;
  REQUIRE_OK(tio.initialize(env.spec().training, env.core()));
  t::PartialExampleReader rdr;
  REQUIRE_OK(rdr.setData(ex));
  REQUIRE_OK(rdr.initialize(&tio));
  t::PartialExample pe;
  bool eof = false;
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK_FALSE(eof);
  CHECK(pe.surface() == "ももも");
  CHECK(pe.nodes().size() == 1);
  CHECK(pe.nodes()[0].tags.empty());
  CHECK(pe.boundaries().size() == 1);
  CHECK(pe.boundaries()[0] == 3);
  REQUIRE_OK(rdr.readExample(&pe, &eof));
  CHECK(pe.surface() == "ももも");
  CHECK(pe.nodes().size() == 2);
  CHECK(pe.nodes()[0].tags.empty());
  CHECK(pe.nodes()[1].tags.empty());
  CHECK(pe.boundaries().size() == 2);
  CHECK(pe.boundaries()[0] == 3);
  CHECK(pe.boundaries()[1] == 4);
  CHECK_FALSE(eof);
  t::PartialExample pe2;
  REQUIRE_OK(rdr.readExample(&pe2, &eof));
  CHECK(eof);
}
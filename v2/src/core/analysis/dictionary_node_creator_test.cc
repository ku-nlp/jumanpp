//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "dictionary_node_creator.h"
#include "core/dic_builder.h"
#include "core/dictionary.h"
#include "core/spec/spec_dsl.h"
#include "testing/standalone_test.h"
#include "util/string_piece.h"

using namespace jumanpp::core::analysis;
using namespace jumanpp::core::spec;
using namespace jumanpp::core::dic;

using jumanpp::StringPiece;

class NodeCreatorTestEnv {
 public:
  AnalysisSpec dicSpec;
  jumanpp::core::dic::DictionaryBuilder dicBldr;
  AnalysisInput input;
  LatticeBuilder latticeBldr;
  EntriesHolder holder;

 public:
  NodeCreatorTestEnv(StringPiece csvData) {
    dsl::ModelSpecBuilder specBldr;
    specBldr.field(1, "a").strings().trieIndex();
    CHECK_OK(specBldr.build(&dicSpec));
    CHECK_OK(dicBldr.importSpec(&dicSpec));
    CHECK_OK(dicBldr.importCsv("dic", csvData));
    CHECK_OK(fillEntriesHolder(dicBldr.result(), &holder));
  }

  void analyze(StringPiece str) {
    DictionaryNodeCreator dnc{DictionaryEntries{&holder}};
    CHECK_OK(input.reset(str));
    latticeBldr.reset(input.numCodepoints());
    dnc.spawnNodes(input, &latticeBldr);
  }
};

TEST_CASE("it is possible to extract some nodes from input string") {
  NodeCreatorTestEnv env{
      "of\na\nan\napple\nabout\nargument\narg\ngum\nmug\nrgu\nfme"};
  env.analyze("argumentofme");
  CHECK_OK(env.latticeBldr.prepare());
  CHECK(env.latticeBldr.seeds().size() == 7);
}
//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"
#include "core/dic/dic_builder.h"
#include "core/dic/dic_entries.h"
#include "core/spec/spec_dsl.h"
#include "field_reader.h"
#include "testing/standalone_test.h"

using namespace jumanpp;
using namespace jumanpp::core::dic;
using namespace jumanpp::core::spec;

class TestStringColumn {
  impl::StringStorageReader rdr_;

 public:
  TestStringColumn(StringPiece data, u32 align) : rdr_{data, align} {}
  StringPiece operator[](i32 pos) {
    StringPiece pc;
    REQUIRE(rdr_.readAt(pos, &pc));
    return pc;
  }
};

struct TesterStep {
  std::vector<TestStringColumn>& cols_;
  IndexTraversal trav;
  std::shared_ptr<IndexedEntries> content;
  DicEntryBuffer entry;

  TesterStep(std::vector<TestStringColumn>& cols_, const IndexTraversal& trav)
      : cols_(cols_), trav(trav) {}

  TesterStep& step(StringPiece sp, TraverseStatus expected) {
    auto actual = trav.step(sp);
    REQUIRE(actual == expected);
    return *this;
  }

  TesterStep& fillEntries() {
    if (content) {
      REQUIRE(content->remaining() == 0);
      *content = trav.entries();
    } else {
      content.reset(new IndexedEntries{trav.entries()});
    }
    return *this;
  }

  TesterStep& strings(std::initializer_list<StringPiece> sp) {
    std::vector<StringPiece> local{std::begin(sp), std::end(sp)};
    CHECK(local.size() == cols_.size());
    REQUIRE(content->readOnePtr());
    REQUIRE(content->fillEntryData(&entry));
    auto entryData = entry.features();
    for (int column = 0; column < local.size(); ++column) {
      CAPTURE(column);
      CHECK(cols_[column][entryData[column]] == local[column]);
    }
    return *this;
  }

  ~TesterStep() {
    if (content) {
      CHECK(content->remaining() == 0);
    }
  }
};

struct DataTester {
  std::vector<TestStringColumn> columns;
  EntriesHolder holder;
  std::unique_ptr<DictionaryEntries> entrs;

  DataTester(const BuiltDictionary& dic) {
    for (auto& x : dic.fieldData) {
      columns.emplace_back(x.stringContent, 0);
    }
    CHECK_OK(fillEntriesHolder(dic, &holder));
    entrs.reset(new DictionaryEntries{&holder});
  }

  TesterStep operator()() { return TesterStep{columns, entrs->traversal()}; }
};

class TesterSpec {
 public:
  AnalysisSpec spec;

  TesterSpec() {
    dsl::ModelSpecBuilder bldr;
    auto& a = bldr.field(1, "a").strings().trieIndex();
    auto& b = bldr.field(2, "b").strings();
    bldr.unigram({a, b});
    CHECK_OK(bldr.build(&spec));
  }
};

TEST_CASE("small dictionary is imported") {
  TesterSpec test;
  StringPiece data{"a,b\nc,d\ne,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&test.spec));
  CHECK_OK(bldr.importCsv("data", data));
  auto& dic = bldr.result();
  CHECK(dic.entryCount == 3);
  CHECK(dic.fieldData.size() == 2);
  CHECK(dic.fieldData[0].uniqueValues == 3);
  CHECK(dic.fieldData[1].uniqueValues == 3);

  DataTester tester{dic};
  tester().step("a", TraverseStatus::Ok).fillEntries().strings({"a", "b"});
  tester().step("c", TraverseStatus::Ok).fillEntries().strings({"c", "d"});
  tester().step("e", TraverseStatus::Ok).fillEntries().strings({"e", "f"});
}

TEST_CASE("small substring-only is imported") {
  TesterSpec test;
  StringPiece data{"abc,b\nab,d\na,f\nabcd,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&test.spec));
  CHECK_OK(bldr.importCsv("data", data));
  auto& dic = bldr.result();
  CHECK(dic.entryCount == 4);
  CHECK(dic.fieldData.size() == 2);
  CHECK(dic.fieldData[0].uniqueValues == 4);
  CHECK(dic.fieldData[1].uniqueValues == 3);

  auto Ok = TraverseStatus::Ok;

  DataTester tester{dic};
  tester()
      .step("a", Ok)
      .fillEntries()
      .strings({"a", "f"})
      .step("b", Ok)
      .fillEntries()
      .strings({"ab", "d"})
      .step("c", Ok)
      .fillEntries()
      .strings({"abc", "b"})
      .step("d", Ok)
      .fillEntries()
      .strings({"abcd", "f"});
}

TEST_CASE(
    "small dictionary where exist field with duplicated entries is imported") {
  TesterSpec test;
  StringPiece data{"a,b\na,d\ne,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&test.spec));
  CHECK_OK(bldr.importCsv("data", data));
  auto& dic = bldr.result();
  CHECK(dic.entryCount == 3);
  CHECK(dic.fieldData.size() == 2);
  CHECK(dic.fieldData[0].uniqueValues == 2);
  CHECK(dic.fieldData[1].uniqueValues == 3);

  DataTester tester{dic};
  tester()
      .step("a", TraverseStatus::Ok)
      .fillEntries()
      .strings({"a", "b"})
      .strings({"a", "d"});
  tester().step("e", TraverseStatus::Ok).fillEntries().strings({"e", "f"});
}

TEST_CASE("small with substrings dictionary is imported") {
  TesterSpec test;
  StringPiece data{"a,b\na,d\nae,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&test.spec));
  CHECK_OK(bldr.importCsv("data", data));
  auto& dic = bldr.result();
  CHECK(dic.entryCount == 3);
  CHECK(dic.fieldData.size() == 2);
  CHECK(dic.fieldData[0].uniqueValues == 2);
  CHECK(dic.fieldData[1].uniqueValues == 3);

  DataTester tester{dic};
  tester()
      .step("a", TraverseStatus::Ok)
      .fillEntries()
      .strings({"a", "b"})
      .strings({"a", "d"})
      .step("e", TraverseStatus::Ok)
      .fillEntries()
      .strings({"ae", "f"})
      .step("x", TraverseStatus::NoNode);
}

TEST_CASE("dictionary with shared storage is imported") {
  dsl::ModelSpecBuilder mb;
  auto& fa = mb.field(1, "a").strings().trieIndex();
  auto& fb = mb.field(2, "b").strings().stringStorage(fa);
  mb.unigram({fa, fb});
  AnalysisSpec spec;
  CHECK_OK(mb.build(&spec));

  StringPiece data{"a,c\na,d\ne,a\ng,a"};
  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&spec));
  CHECK_OK(bldr.importCsv("data", data));
  auto& dic = bldr.result();
  CHECK(dic.entryCount == 4);
  CHECK(dic.fieldData.size() == 2);
  CHECK(dic.fieldData[0].uniqueValues == 5);
  CHECK(dic.fieldData[1].uniqueValues == 5);
  CHECK(dic.fieldData[0].stringContent == dic.fieldData[1].stringContent);
  CHECK(dic.fieldData[0].stringContent.size() == 11);

  DataTester tester{dic};
  tester()
      .step("a", TraverseStatus::Ok)
      .fillEntries()
      .strings({"a", "c"})
      .strings({"a", "d"})
      .step("x", TraverseStatus::NoNode);
}

TEST_CASE("small dictionary where one of lines is corrupted is not imported") {
  TesterSpec test;
  StringPiece data{"a,b\nd\ne,f"};
  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&test.spec));
  auto status = bldr.importCsv("data", data);
  CHECK_FALSE(status);
  CHECK_THAT(status.message().str(), Catch::Contains("there were 1 columns"));
  CHECK_THAT(status.message().str(), Catch::Contains("on line 2"));
}

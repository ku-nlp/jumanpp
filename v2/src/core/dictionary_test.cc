//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "core/dictionary.h"
#include "core/dic_builder.h"
#include "core/dic_entries.h"
#include "core/impl/field_reader.h"
#include "core/spec/spec_dsl.h"
#include "testing/standalone_test.h"

using namespace jumanpp;
using namespace jumanpp::core::dic;
using namespace jumanpp::core::spec;

using jumanpp::core::TraverseStatus;

class TestStringColumn {
  impl::StringStorageReader rdr_;

 public:
  TestStringColumn(StringPiece data) : rdr_{data} {}
  StringPiece operator[](i32 pos) {
    StringPiece pc;
    REQUIRE(rdr_.readAt(pos, &pc));
    return pc;
  }
};

struct TesterStep {
  std::vector<TestStringColumn>& cols_;
  IndexTraversal trav;
  std::unique_ptr<IndexedEntries> content;

  TesterStep(std::vector<TestStringColumn>& cols_, const IndexTraversal& trav)
      : cols_(cols_), trav(trav) {}
  TesterStep(TesterStep&& other)
      : cols_{other.cols_},
        trav{other.trav},
        content{std::move(other.content)} {}

  TesterStep step(StringPiece sp, TraverseStatus expected) {
    auto actual = trav.step(sp);
    REQUIRE(actual == expected);
    return std::move(*this);
  }

  TesterStep fillEntries() {
    if (content) {
      REQUIRE(content->remaining() == 0);
      *content = std::move(trav.entries());
    } else {
      content.reset(new IndexedEntries{trav.entries()});
    }
    return std::move(*this);
  }

  TesterStep strings(std::initializer_list<StringPiece> sp) {
    std::vector<StringPiece> local{std::begin(sp), std::end(sp)};
    CHECK(local.size() == cols_.size());
    std::vector<i32> entryData;
    entryData.resize(cols_.size());
    util::MutableArraySlice<i32> slice(&entryData);
    content->fillEntryData(&slice);
    for (int column = 0; column < local.size(); ++column) {
      CAPTURE(column);
      CHECK(cols_[column][entryData[column]] == local[column]);
    }
    return std::move(*this);
  }

  ~TesterStep() {
    if (content) {
      REQUIRE(content->remaining() == 0);
    }
  }
};

struct DataTester {
  std::vector<TestStringColumn> columns;
  EntriesHolder holder;
  std::unique_ptr<DictionaryEntries> entrs;

  DataTester(const BuiltDictionary& dic) {
    for (auto& x : dic.fieldData) {
      columns.emplace_back(x.stringContent);
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
    bldr.field(1, "a").strings().trieIndex();
    bldr.field(2, "b").strings();
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

TEST_CASE("small dictionary where one of lines is corrupted is not imported") {
  TesterSpec test;
  StringPiece data{"a,b\nd\ne,f"};
  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec(&test.spec));
  auto status = bldr.importCsv("data", data);
  CHECK_FALSE(status);
  CHECK(status.message.find("there were 1 columns") != std::string::npos);
  CHECK(status.message.find("on line 2") != std::string::npos);
}

//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"
#include <testing/standalone_test.h>
#include "dic_builder.h"
#include "dic_entries.h"
#include "impl/field_reader.h"

using namespace jumanpp;
using namespace jumanpp::core::dic;
using jumanpp::core::TraverseStatus;

void fillHolder(const BuiltDictionary& dic, EntriesHolder* result) {
  result->entrySize = static_cast<i32>(dic.fieldData.size());
  result->entries = impl::IntStorageReader{dic.entryData};
  result->entryPtrs = impl::IntStorageReader{dic.entryPointers};
  CHECK_OK(result->trie.loadFromMemory(dic.trieContent));
}

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
    std::vector<StringPiece> local{sp};
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
    fillHolder(dic, &holder);
    entrs.reset(new DictionaryEntries{&holder});
  }

  TesterStep operator()() { return TesterStep{columns, entrs->traversal()}; }
};

TEST_CASE("small dictionary is imported") {
  StringPiece spec{"1 a string trie_index\n2 b string"};
  StringPiece data{"a,b\nc,d\ne,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec("spec", spec));
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

TEST_CASE(
    "small dictionary where exist field with duplicated entries is imported") {
  StringPiece spec{"1 a string trie_index\n2 b string"};
  StringPiece data{"a,b\na,d\ne,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec("spec", spec));
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
  StringPiece spec{"1 a string trie_index\n2 b string"};
  StringPiece data{"a,b\na,d\nae,f"};

  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec("spec", spec));
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
  StringPiece spec{"1 a string trie_index\n2 b string"};
  StringPiece data{"a,b\nd\ne,f"};
  DictionaryBuilder bldr;
  CHECK_OK(bldr.importSpec("spec", spec));
  auto status = bldr.importCsv("data", data);
  CHECK_FALSE(status);
  CHECK(status.message.find("there were 1 columns") != std::string::npos);
  CHECK(status.message.find("on line 2") != std::string::npos);
}

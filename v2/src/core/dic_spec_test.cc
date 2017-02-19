//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dic_spec.h"
#include <testing/standalone_test.h>

namespace d = jumanpp::core::dic;

TEST_CASE("can parse single line spec") {
  d::DictionarySpec spec{};
  CHECK_OK(d::parseDescriptor("1 SURFACE STRING TRIE_INDEX", &spec));
  REQUIRE(spec.columns.size() == 1);
  auto& first = spec.columns[0];
  CHECK(first.position == 1);
  CHECK(first.name == "SURFACE");
  CHECK(first.isTrieKey);
  CHECK(first.columnType == d::ColumnType::String);
}

TEST_CASE("can parse two line spec with comment") {
  auto&& data = R"(
    5 testx int
    6 tesz string #and comment here
    9 whatever string_list
    12 good string trie_index #
  )";
  d::DictionarySpec spec{};
  CHECK_OK(d::parseDescriptor(data, &spec));
  REQUIRE(spec.columns.size() == 4);
  auto& first = spec.columns[0];
  CHECK(first.position == 5);
  CHECK(first.name == "testx");
  CHECK_FALSE(first.isTrieKey);
  CHECK(first.columnType == d::ColumnType::Int);
  auto& second = spec.columns[1];
  CHECK(second.position == 6);
  CHECK(second.name == "tesz");
  CHECK_FALSE(second.isTrieKey);
  CHECK(second.columnType == d::ColumnType::String);
  auto& third = spec.columns[2];
  CHECK(third.position == 9);
  CHECK(third.name == "whatever");
  CHECK_FALSE(third.isTrieKey);
  CHECK(third.columnType == d::ColumnType::StringList);
  auto& fourth = spec.columns[3];
  CHECK(fourth.position == 12);
  CHECK(fourth.name == "good");
  CHECK(fourth.isTrieKey);
  CHECK(fourth.columnType == d::ColumnType::String);
}

TEST_CASE("check fails when there no indexed columns") {
  d::DictionarySpec spec{};
  auto s = d::parseDescriptor("1 SURFACE STRING", &spec);
  CHECK_FALSE(s.isOk());
  CHECK(s.message == "there was no column to be indexed");
}

TEST_CASE("check fails when there are two indexed columns") {
  d::DictionarySpec spec{};
  auto s = d::parseDescriptor(
      "1 SURFACE STRING TRIE_INDEX\n2 SURFACE2 STRING TRIE_INDEX", &spec);
  CHECK_FALSE(s.isOk());
  CHECK(s.message == "only one column can be indexed, 2 found");
}

TEST_CASE("columns should have unique names") {
  d::DictionarySpec spec{};
  auto s = d::parseDescriptor("1 SURFACE STRING TRIE_INDEX\n2 SURFACE STRING",
                              &spec);
  CHECK_FALSE(s.isOk());
  CHECK(s.message ==
        "name SURFACE was used at least for two columns. "
        "Column names must be unique");
}
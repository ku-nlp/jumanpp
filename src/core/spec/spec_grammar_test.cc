//
// Created by Arseny Tolmachev on 2017/02/22.
//

#include "core/spec/spec_grammar.h"
#include <pegtl/analyze.hpp>
#include <pegtl/contrib/tracer.hpp>
#include <pegtl/parse.hpp>
#include <pegtl/tracking_mode.hpp>
#include "testing/standalone_test.h"
#include "util/string_piece.h"

using namespace jumanpp;
namespace p = jumanpp::core::spec::parser;
namespace pt = jumanpp::core::spec::parser::p;

template <typename T>
void analyzeRule() {
  const size_t numIssues = pt::analyze<T>(true);
  REQUIRE(numIssues == 0);
}

template <typename T>
void shouldParse(StringPiece data) {
  struct full_grammar : pt::must<T, pt::eof> {};
  pt::memory_input<> input(data.char_begin(), data.char_end(), "test");
  bool success = false;
  try {
    success = pt::parse<full_grammar>(input);
  } catch (pt::parse_error& e) {
    success = false;
  }

  CAPTURE(data);
  if (!success) {
    input.restart();
    REQUIRE(pt::parse<full_grammar, pt::nothing, pt::tracer>(input));
  }
}

template <typename T>
void failParse(StringPiece data) {
  struct full_grammar : pt::must<T, pt::eof> {};
  pt::memory_input<> input(data.char_begin(), data.char_end(), "test");
  auto result = false;
  try {
    result = pt::parse<full_grammar>(input);
  } catch (pt::parse_error& e) {
    result = false;
  }
  CAPTURE(data);
  CHECK(!result);
}

TEST_CASE("grammar supports identifier") {
  analyzeRule<p::ident>();
  shouldParse<p::ident>("test");
  shouldParse<p::ident>("test_with_separators");
  failParse<p::ident>("test test2");
  failParse<p::ident>("");
  failParse<p::ident>("help\"me");
  failParse<p::ident>("help\tme");
  failParse<p::ident>("help+me");
  failParse<p::ident>("help=me");
}

TEST_CASE("grammar supports numbers") {
  shouldParse<p::number>("1");
  shouldParse<p::number>("99999");
  failParse<p::number>("999991");
  failParse<p::number>("a1");
  failParse<p::number>("1a");
  failParse<p::number>("");
}

TEST_CASE("grammar keywords") {
  shouldParse<p::kw_field>("field");
  shouldParse<p::kw_field>("FIELD");
  shouldParse<p::kw_unk>("unknown");
  shouldParse<p::kw_unk>("UNKNOWN");
  shouldParse<p::kw_feature>("feature");
  shouldParse<p::kw_feature>("FEATURE");
  shouldParse<p::kw_unigram>("unigram");
  shouldParse<p::kw_unigram>("UNIGRAM");
  shouldParse<p::kw_bigram>("bigram");
  shouldParse<p::kw_bigram>("BIGRAM");
  shouldParse<p::kw_trigram>("trigram");
  shouldParse<p::kw_trigram>("TRIGRAM");
  shouldParse<p::kw_data>("data");
  shouldParse<p::kw_data>("DATA");
}

TEST_CASE("quotes string parses") {
  shouldParse<p::qstring>("\"\"");
  shouldParse<p::qstring>("\"ba\"");
  shouldParse<p::qstring>("\"\"\"ba\"");
  shouldParse<p::qstring>("\"ba\"\"ba\"");
  shouldParse<p::qstring>("\"ba\"\"\"");
  failParse<p::qstring>("\"ba\"\"");
}

TEST_CASE("field content parses") {
  analyzeRule<p::fld_data>();
  shouldParse<p::fld_data>("1 name string");
  shouldParse<p::fld_data>("1 name string_list");
  shouldParse<p::fld_data>("1 name int");
  shouldParse<p::fld_data>("1 name int trie_index");
  shouldParse<p::fld_data>("1 name int empty \"nil\"");
  shouldParse<p::fld_data>("1 name int empty \"nil\" trie_index");
  shouldParse<p::fld_data>("1 name int trie_index empty \"nil\"");
  failParse<p::fld_data>("1 name int empty trie_index \"nil\"");
}

TEST_CASE("full field parses") {
  analyzeRule<p::fld_stmt>();
  shouldParse<p::fld_stmt>("field 1 name string");
  shouldParse<p::fld_stmt>("field 1 name string_list");
  shouldParse<p::fld_stmt>("field 1 name int");
  shouldParse<p::fld_stmt>("field 1 name kv_list");
}

TEST_CASE("can parse match condition") {
  analyzeRule<p::mt_cond>();
  shouldParse<p::mt_cond>("x with \"y\"");
  shouldParse<p::mt_cond>("[z, x] with \"y\"");
  shouldParse<p::mt_cond>("[ z , x ] with \"y\"");
  shouldParse<p::mt_cond>("[z,x]with\"y\"");
  shouldParse<p::mt_cond>("[z,x] with file \"y\"");
  shouldParse<p::mt_cond>("[z,x] with file\"y\"");
}

TEST_CASE("can parse match expression") {
  analyzeRule<p::ft_match>();
  shouldParse<p::ft_match>("match x with \"y\"");
  shouldParse<p::ft_match>("match x with \"y\" then [a] else [b]");
  failParse<p::ft_match>("match x with \"y\" then [a]");
}

TEST_CASE("can parse unigram combiner") {
  analyzeRule<p::ngram_stmt>();
  shouldParse<p::ngram_stmt>("ngram [a]");
  shouldParse<p::ngram_stmt>("ngram [a,b]");
  shouldParse<p::ngram_stmt>("ngram [f1, a]");
  shouldParse<p::ngram_stmt>("ngram [ a, b ]");
  shouldParse<p::ngram_stmt>("ngram[f1,b]");
  shouldParse<p::ngram_stmt>("ngram[f1,b][f2]");
  shouldParse<p::ngram_stmt>("ngram[f1,b][f2] [f3]");
  shouldParse<p::ngram_stmt>("ngram[f1,b][ f2] [f3]");
  failParse<p::ngram_stmt>("ngram[f1,b][f2] [f3][f4]");
  failParse<p::ngram_stmt>("ngram");
}

TEST_CASE("can parse char type expression") {
  analyzeRule<p::char_type_expr>();
  shouldParse<p::char_type_expr>("a");
  shouldParse<p::char_type_expr>("a | b");
  shouldParse<p::char_type_expr>("a | b|c");
}

TEST_CASE("can parse unk handlers") {
  analyzeRule<p::unk_def>();
  shouldParse<p::unk_def>(
      "unk kata template row 1: single katakana surface to [surface]");
  shouldParse<p::unk_def>(
      "unk kata template row 1: chunking katakana surface to [surface]");
  shouldParse<p::unk_def>(
      "unk kata template row 1: onomatopeia katakana surface to [surface]");
  shouldParse<p::unk_def>(
      "unk kata template row 1: numeric katakana surface to [surface]");
  shouldParse<p::unk_def>(
      "unk kata template row 1: normalize surface to [surface]");
  shouldParse<p::unk_def>(
      "unk aaabbb template row 1: normalize surface to [surface] feature to "
      "test");
  shouldParse<p::unk_def>(
      "unk alphabet template row 5: chunking FAMILY_ALPH surface to [surface, "
      "baseform, reading] feature to notPrefix");
}

TEST_CASE("can parse train field definitions") {
  analyzeRule<p::train_field>();
  shouldParse<p::train_field>("a 1");
  shouldParse<p::train_field>("a:1");
  shouldParse<p::train_field>("a :1");
  shouldParse<p::train_field>("a: 1");
  shouldParse<p::train_field>("a: 1.5121");
}

TEST_CASE("can parse floating point constants") {
  analyzeRule<p::floatconst>();
  shouldParse<p::floatconst>("0");
  shouldParse<p::floatconst>("01");
  shouldParse<p::floatconst>("01.151");
  shouldParse<p::floatconst>("021.151");
  shouldParse<p::floatconst>("+021.151");
  shouldParse<p::floatconst>("-021.151");
}

TEST_CASE("can parse unk_gold_if") {
  analyzeRule<p::train_gold_unk>();
  shouldParse<p::train_gold_unk>("unk_gold_if features[\"品詞推定\"] == pos");
}

TEST_CASE("analyze full grammar") { analyzeRule<p::full_spec>(); }

TEST_CASE("analyze full grammar: two ngrams") {
  shouldParse<p::full_spec>("ngram [a]\nngram[b]");
}
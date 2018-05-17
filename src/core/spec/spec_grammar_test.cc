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

TEST_CASE("can parse argument") {
  shouldParse<p::arg>("dicform");
  shouldParse<p::arg>("dicform=1");
  shouldParse<p::arg>("dicform+=1");
  shouldParse<p::arg>("dicform+=\"1\"");
  shouldParse<p::arg>("dicform=\"1\"");
  shouldParse<p::arg>("dicform =\"1\"");
  shouldParse<p::arg>("dicform = \"1\"");
  shouldParse<p::arg>("dicform += 1");
  failParse<p::arg>("dicform += ");
  failParse<p::arg>("dicform = ");
}

TEST_CASE("can parse arglist") {
  shouldParse<p::arglist>("[a]");
  shouldParse<p::arglist>("[a ]");
  shouldParse<p::arglist>("[ a]");
  shouldParse<p::arglist>("[a,b]");
  shouldParse<p::arglist>("[a ,b]");
  shouldParse<p::arglist>("[a, b]");
  shouldParse<p::arglist>("[a , b]");
  shouldParse<p::arglist>("[a,b,c,d]");
  shouldParse<p::arglist>("[a,b,c=2,d]");
  shouldParse<p::arglist>("[a ,b,c=2,d]");
  shouldParse<p::arglist>("[a ,b,c=2,d ]");
  shouldParse<p::arglist>("[a ,b,c= 2,d ]");
  shouldParse<p::arglist>("[a ,b+=\"1\",c= 2,d ]");
}

TEST_CASE("can parse unk spec") {
  shouldParse<p::unk_data>("unknown a template 1 output[c] feature[z]");
  shouldParse<p::unk_data>(
      "unknown a template 1 output[c,a] feature[z,a=5,b+=7]");
  failParse<p::unk_data>("unknown a template z output[c] feature[z]");
  failParse<p::unk_data>("unknown a template z output[c]");
  failParse<p::unk_data>("unknown a template z");
  failParse<p::unk_data>("unknown a template z");
  failParse<p::unk_data>("unknown a template z fea");
  failParse<p::unk_data>("unknown a template z feature");
  failParse<p::unk_data>("unknown a template z feature[z]");
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
  analyzeRule<p::ngram_uni>();
  shouldParse<p::ngram_uni>("unigram [a]");
  shouldParse<p::ngram_uni>("unigram [a,b]");
  shouldParse<p::ngram_uni>("unigram [f1, a]");
  shouldParse<p::ngram_uni>("unigram [ a, b ]");
  shouldParse<p::ngram_uni>("unigram[f1,b]");
}

TEST_CASE("can parse condition of if expression") {
  shouldParse<p::if_cond>("(x match [a])");
  shouldParse<p::if_cond>("(x match [a] )");
  shouldParse<p::if_cond>("( x match [a] )");
}

TEST_CASE("can parse if expression") {
  shouldParse<p::if_expr>("if (a match [b]) [c] else [d]");
  shouldParse<p::if_expr>("if(a match[b])[c]else[d]");
}

TEST_CASE("can parse feature") {
  shouldParse<p::feat_data>("feature x a=2");
  shouldParse<p::feat_data>("feature x a=\"2\"");
  shouldParse<p::feat_data>("feature x if(a match[b])[c]else[d]");
}

TEST_CASE("can parse unigram feature combination") {
  shouldParse<p::uni_data>("unigram [x]");
  shouldParse<p::uni_data>("unigram[x]");
  failParse<p::uni_data>("unigram[x][y]");
}

TEST_CASE("can parse bigram feature combination") {
  shouldParse<p::bi_data>("bigram [x][y]");
  shouldParse<p::bi_data>("bigram [x] [y]");
  shouldParse<p::bi_data>("bigram[x] [y]");
  failParse<p::bi_data>("bigram[x]");
  failParse<p::bi_data>("bigram[x][y][z]");
}

TEST_CASE("can parse trigram feature combination") {
  shouldParse<p::tri_data>("trigram [x][y][z]");
  shouldParse<p::tri_data>("trigram [x][y] [z]");
  shouldParse<p::tri_data>("trigram [x] [y] [z]");
  shouldParse<p::tri_data>("trigram[x] [y] [z]");
  shouldParse<p::tri_data>("trigram[x] [y][z]");
  shouldParse<p::tri_data>("trigram[x][y][z]");
  shouldParse<p::tri_data>("trigram[x][y] [z]");
  failParse<p::tri_data>("trigram[x][y]");
  failParse<p::tri_data>("trigram[x]");
}
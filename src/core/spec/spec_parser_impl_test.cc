//
// Created by Arseny Tolmachev on 2018/05/16.
//

#include "spec_parser_impl.h"
#include "testing/standalone_test.h"

#include <pegtl/parse.hpp>
#include <pegtl/parse_error.hpp>
#include "util/logging.hpp"

using namespace jumanpp;
namespace p = jumanpp::core::spec::parser;
namespace pt = jumanpp::core::spec::parser::p;
namespace dsl = jumanpp::core::spec::dsl;
namespace s = jumanpp::core::spec;

template <typename Rule>
void parse(StringPiece data, p::SpecParserImpl& sp, bool target = true) {
  bool success = false;
  CAPTURE(pt::internal::demangle<Rule>());
  struct theRule : pt::must<Rule, pt::eof> {};
  try {
    pt::memory_input<> input(data.char_begin(), data.char_end(), "test");
    success = pt::parse<theRule, p::SpecAction>(input, sp);
  } catch (pt::parse_error& e) {
    if (target) {
      LOG_WARN() << e.what();
    }
    success = false;
  }
  REQUIRE(success == target);
}

TEST_CASE("spec parser parses field declaration") {
  p::SpecParserImpl spi{"test.spec"};

  SECTION("simple string param") {
    parse<p::fld_stmt>("field 66 testxa string", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    CHECK(fld->name() == "testxa");
    CHECK(fld->getCsvColumn() == 66);
    CHECK(fld->getColumnType() == s::FieldType::String);
  }

  SECTION("reject two fields with same names") {
    parse<p::fld_stmt>("field 66 testxa string", spi);
    parse<p::fld_stmt>("field 66 testxa string", spi, false);
  }

  SECTION("simple int param") {
    parse<p::fld_stmt>("field 66 testxa int", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    CHECK(fld->getColumnType() == s::FieldType::Int);
  }

  SECTION("simple slist param") {
    parse<p::fld_stmt>("field 66 testxa string_list", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    CHECK(fld->getColumnType() == s::FieldType::StringList);
  }

  SECTION("simple kvlist param") {
    parse<p::fld_stmt>("field 66 testxa kv_list", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    CHECK(fld->getColumnType() == s::FieldType::StringKVList);
  }

  SECTION("trie key works") {
    parse<p::fld_stmt>("field 66 testxa string trie_index", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    StringPiece sstor;
    s::FieldDescriptor fd;
    REQUIRE(fld->fill(&fd, &sstor));
    CHECK(fd.isTrieKey);
  }

  SECTION("empty value works") {
    parse<p::fld_stmt>("field 66 testxa string empty \"*\"", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    StringPiece sstor;
    s::FieldDescriptor fd;
    REQUIRE(fld->fill(&fd, &sstor));
    CHECK(fd.emptyString == "*");
  }

  SECTION("alignment works") {
    parse<p::fld_stmt>("field 66 testxa string align 5", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    StringPiece sstor;
    s::FieldDescriptor fd;
    REQUIRE(fld->fill(&fd, &sstor));
    CHECK(fd.alignment == 5);
  }

  SECTION("list separator works") {
    parse<p::fld_stmt>("field 66 testxa string_list list_sep \"6\"", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    StringPiece sstor;
    s::FieldDescriptor fd;
    REQUIRE(fld->fill(&fd, &sstor));
    CHECK(fd.listSeparator == "6");
  }

  SECTION("kv separator works") {
    parse<p::fld_stmt>("field 66 testxa kv_list kv_sep \"12\"", spi);
    auto fld = spi.fields_["testxa"];
    REQUIRE(fld != nullptr);
    StringPiece sstor;
    s::FieldDescriptor fd;
    REQUIRE(fld->fill(&fd, &sstor));
    CHECK(fd.kvSeparator == "12");
  }
}

s::AnalysisSpec build(p::SpecParserImpl& impl) {
  s::AnalysisSpec res;
  REQUIRE(impl.buildSpec(&res));
  return res;
}

TEST_CASE("spec parser parses feature declaration") {
  p::SpecParserImpl spi{"test.spec"};
  parse<p::fld_stmt>("field 1 a string trie_index", spi);
  parse<p::fld_stmt>("field 2 b string", spi);
  parse<p::fld_stmt>("field 3 c string", spi);
  parse<p::fld_stmt>("field 4 d string_list", spi);
  parse<p::fld_stmt>("field 5 e kv_list", spi);

  SECTION("placeholder") {
    parse<p::feature_stmt>("feature f1 = placeholder", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 2);
    CHECK(spec.features.primitive[0].kind == s::PrimitiveFeatureKind::Provided);
  }

  SECTION("numCodepoints") {
    parse<p::feature_stmt>("feature f1 = num_codepoints a", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 2);
    CHECK(spec.features.primitive[0].kind ==
          s::PrimitiveFeatureKind::SurfaceCodepointSize);
  }

  SECTION("numCodepoints non-surface") {
    parse<p::feature_stmt>("feature f1 = num_codepoints b", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 3);
    CHECK(spec.features.primitive[0].kind ==
          s::PrimitiveFeatureKind::CodepointSize);
  }

  SECTION("length") {
    parse<p::feature_stmt>("feature f1 = num_bytes a", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 2);
    CHECK(spec.features.primitive[0].kind ==
          s::PrimitiveFeatureKind::ByteLength);
  }

  SECTION("codepoint") {
    parse<p::feature_stmt>("feature f1 = codepoint -2", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 2);
    CHECK(spec.features.primitive[0].kind ==
          s::PrimitiveFeatureKind::Codepoint);
    REQUIRE(spec.features.primitive[0].references.at(0) == -2);
  }

  SECTION("codepointType") {
    parse<p::feature_stmt>("feature f1 = codepoint_type -2", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 2);
    CHECK(spec.features.primitive[0].kind ==
          s::PrimitiveFeatureKind::CodepointType);
    REQUIRE(spec.features.primitive[0].references.at(0) == -2);
  }

  SECTION("match condition (a)") {
    parse<p::feature_stmt>("feature f1 = match b with \"x\"", spi);
    parse<p::ngram_uni>("unigram [f1, a]", spi);
    auto spec = build(spi);
    CHECK(spec.features.primitive.size() == 3);
    CHECK(spec.features.primitive[0].kind ==
          s::PrimitiveFeatureKind::SingleBit);
    CHECK(spec.features.primitive[0].references.at(0) == 2);
    CHECK(spec.features.primitive[0].references.at(1) == 0);
    CHECK(spec.features.dictionary[2].kind == s::DicImportKind::MatchFields);
    CHECK(spec.features.dictionary[2].data.at(0) == "x");
  }
}
//
// Created by Arseny Tolmachev on 2018/05/16.
//

#include "spec_parser_impl.h"
#include "util/mmap.h"
#include "path.hpp"

namespace jumanpp {
namespace core {
namespace spec {
namespace parser {

Status SpecParserImpl::buildSpec(AnalysisSpec *result) {
  JPP_RETURN_IF_ERROR(builder_.build(result));
  return Status::Ok();
}

struct FileResource: Resource {
  util::FullyMappedFile file_;
  Status open(StringPiece name) {
    return file_.open(name, util::MMapType::ReadOnly);
  }

  Status validate() const override {
    return Status::Ok();
  }

  StringPiece data() const override {
    return file_.contents();
  }
};

Resource *SpecParserImpl::fileResourece(StringPiece name, p::position pos) {
  Pathie::Path p{this->basename_};
  auto par = p.parent();
  auto theName = par.join(name.str());
  FileResource* res = builder_.alloc_->make<FileResource>();
  builder_.garbage_.push_back(res);
  auto nameString = theName.str();
  auto s = res->open(nameString);
  if (!s) {
    throw p::parse_error("failed to open file: [" + nameString + "], error: " + s.message().str(), pos);
  }
  return res;
}

chars::CharacterClass charClassByName(StringPiece name) {
  using C = chars::CharacterClass;
  using S = StringPiece;

  static util::FlatMap<StringPiece, C> classes{
      // clang-format off
    {S{"SPACE"}, C::SPACE},
    {S{"IDEOGRAPHIC_PUNC"}, C::IDEOGRAPHIC_PUNC},
    {S{"KANJI"}, C::KANJI},
    {S{"FIGURE"}, C::FIGURE},
    {S{"PERIOD"}, C::PERIOD},
    {S{"MIDDLE_DOT"}, C::MIDDLE_DOT},
    {S{"COMMA"}, C::COMMA},
    {S{"ALPH"}, C::ALPH},
    {S{"SYMBOL"}, C::SYMBOL},
    {S{"KATAKANA"}, C::KATAKANA},
    {S{"HIRAGANA"}, C::HIRAGANA},
    {S{"KANJI_FIGURE"}, C::KANJI_FIGURE},
    {S{"SLASH"}, C::SLASH},
    {S{"COLON"}, C::COLON},
    {S{"ERA"}, C::ERA},
    {S{"CHOON"}, C::CHOON},
    {S{"HANKAKU_KANA"}, C::HANKAKU_KANA},
    {S{"BRACKET"}, C::BRACKET},
    {S{"FIGURE_EXCEPTION"}, C::FIGURE_EXCEPTION},
    {S{"FIGURE_DIGIT"}, C::FIGURE_DIGIT},
    {S{"SMALL_KANA"}, C::SMALL_KANA},
    {S{"FAMILY_FIGURE"}, C::FAMILY_FIGURE},
    {S{"FAMILY_PUNC"}, C::FAMILY_PUNC},
    {S{"FAMILY_ALPH_PUNC"}, C::FAMILY_ALPH_PUNC},
    {S{"FAMILY_NUM_PERIOD"}, C::FAMILY_NUM_PERIOD},
    {S{"FAMILY_PUNC_SYMBOL"}, C::FAMILY_PUNC_SYMBOL},
    {S{"FAMILY_SPACE"}, C::FAMILY_SPACE},
    {S{"FAMILY_SYMBOL"}, C::FAMILY_SYMBOL},
    {S{"FAMILY_ALPH"}, C::FAMILY_ALPH},
    {S{"FAMILY_KANJI"}, C::FAMILY_KANJI},
    {S{"FAMILY_KANA"}, C::FAMILY_KANA},
    {S{"FAMILY_DOUBLE"}, C::FAMILY_DOUBLE},
    {S{"FAMILY_BRACKET"}, C::FAMILY_BRACKET},
    {S{"FAMILY_DIGITS"}, C::FAMILY_DIGITS},
    {S{"FAMILY_EXCEPTION"}, C::FAMILY_EXCEPTION},
    {S{"FAMILY_PROLONGABLE"}, C::FAMILY_PROLONGABLE},
    {S{"FAMILY_FULL_KANA"}, C::FAMILY_FULL_KANA},
    {S{"FAMILY_OTHERS"}, C::FAMILY_OTHERS},
    {S{"FAMILY_ANYTHING"}, C::FAMILY_ANYTHING},
      // clang-format on
  };

  std::string name_lower;
  for (auto c : name) {
    name_lower.push_back(static_cast<char>(std::toupper(static_cast<char>(c))));
  }

  auto it = classes.find(name_lower);
  if (it != classes.end()) {
    return it->second;
  }
  return C::FAMILY_OTHERS;
}
}  // namespace parser
}  // namespace spec
}  // namespace core
}  // namespace jumanpp
//
// Created by Arseny Tolmachev on 2018/05/16.
//

#ifndef JUMANPP_SPEC_PARSER_IMPL_H
#define JUMANPP_SPEC_PARSER_IMPL_H

#include "spec_dsl.h"
#include "spec_grammar.h"
#include "util/char_buffer.h"
#include "util/flatmap.h"
#include "util/parse_utils.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace spec {
namespace parser {

struct Resource : public dsl::DslOpBase {
  virtual StringPiece data() const = 0;
};

struct SpecParserImpl {
  std::string basename_;
  dsl::ModelSpecBuilder builder_;
  util::FlatMap<StringPiece, dsl::FieldBuilder*> fields_;
  util::FlatMap<StringPiece, dsl::FeatureBuilder*> features_;
  util::CharBuffer<> charBuffer_;

  i32 intParam_;
  StringPiece stringParam_;
  float floatParam_;

  // field data
  dsl::FieldBuilder* curFld_ = nullptr;
  dsl::FeatureBuilder* curFeature_ = nullptr;
  dsl::UnkProcBuilder* curUnk_ = nullptr;

  std::vector<dsl::FieldReference> fieldRefs_;
  std::vector<dsl::FeatureRef> featureRefs_;
  std::vector<std::vector<dsl::FeatureRef>> featureRefRefs_;
  std::vector<chars::CharacterClass> charClassStack_;

  Resource* fileResourece(StringPiece name, p::position pos);
  Status buildSpec(spec::AnalysisSpec* result);

  SpecParserImpl(StringPiece basename) : basename_{basename.str()} {}
};

chars::CharacterClass charClassByName(StringPiece name);

template <typename Rule>
struct SpecAction : p::nothing<Rule> {};

struct StoreToIntParam {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    u64 val = 0;
    if (!util::parseU64(StringPiece{in.begin(), in.end()}, &val)) {
      throw p::parse_error("failed to parse int", in);
    }
    sp.intParam_ = static_cast<i32>(val);
  }
};

struct StoreToSIntParam {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    u64 val = 0;
    bool negative = false;
    auto inp = StringPiece{in.begin(), in.end()};
    if (inp[0] == '-') {
      negative = true;
      inp = inp.from(1);
    }
    if (!util::parseU64(inp, &val)) {
      throw p::parse_error("failed to parse int", in);
    }
    sp.intParam_ = negative ? -static_cast<i32>(val) : static_cast<i32>(val);
  }
};

struct StoreToStringParam {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.stringParam_ = StringPiece{in.begin(), in.end()};
  }
};

struct StoreQuotedStringParam {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto quoted = StringPiece{in.begin() + 1, in.end() - 1};  // ignore quotes

    bool hasQuotes = false;
    for (auto c : quoted) {
      if (c == '"') {
        hasQuotes = true;
        break;
      }
    }

    if (!hasQuotes) {
      sp.stringParam_ = quoted;
      return;
    }

    util::MutableArraySlice<char> chars;
    if (!sp.charBuffer_.importMutable(quoted, &chars)) {
      throw p::parse_error("failed to import quoted string", in);
    }

    int start = 0;
    auto end = chars.size();
    for (; start < end; ++start) {
      if (chars[start] == '"') {
        break;
      }
    }

    int head = start + 1;
    int tail = start;
    while (head < end) {
      chars[tail] = chars[head];
      if (chars[head] == '"') {
        ++tail;
      }
      ++head;
      ++tail;
    }

    sp.stringParam_ = StringPiece{chars.data(), static_cast<size_t>(tail)};
  }
};

template <>
struct SpecAction<fld_column> : StoreToIntParam {};

template <>
struct SpecAction<snumparam> : StoreToSIntParam {};

template <>
struct SpecAction<fld_name> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto name = StringPiece{in.begin(), in.end()};
    auto& fld = sp.builder_.field(sp.intParam_, name);
    sp.curFld_ = &fld;
    auto result = sp.fields_.insert({name, sp.curFld_});
    if (!result.second) {
      throw p::parse_error(
          "a field with name " + name.str() + " was already defined", in);
    }
  }
};

template <>
struct SpecAction<fld_tp_kvlist> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->kvLists();
  }
};

template <>
struct SpecAction<fld_tp_slist> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->stringLists();
  }
};

template <>
struct SpecAction<fld_tp_string> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->strings();
  }
};

template <>
struct SpecAction<fld_tp_int> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->integers();
  }
};

template <>
struct SpecAction<fld_flag_index> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->trieIndex();
  }
};

template <>
struct SpecAction<fld_flag_align_data> : StoreToIntParam {};

template <>
struct SpecAction<fld_flag_align> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->align(sp.intParam_);
  }
};

template <>
struct SpecAction<qstring_param> : StoreQuotedStringParam {};

template <>
struct SpecAction<fld_flag_empty> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_->emptyValue(sp.stringParam_);
  }
};

template <>
struct SpecAction<strparam> : StoreToStringParam {};

template <>
struct SpecAction<fld_flag_storage> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto name = sp.stringParam_;
    auto it = sp.fields_.find(name);
    if (it == sp.fields_.end()) {
      throw p::parse_error(
          std::string("field [") + name.str() + "] does not exist", in);
    }
    sp.curFld_->stringStorage(*it->second);
  }
};

template <>
struct SpecAction<fld_flag_sep> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto sep = sp.stringParam_;
    sp.curFld_->listSeparator(sep);
  }
};

template <>
struct SpecAction<fld_flag_kvsep> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto sep = sp.stringParam_;
    sp.curFld_->kvSeparator(sep);
  }
};

template <>
struct SpecAction<fld_stmt> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFld_ = nullptr;
  }
};

// FEATURES

template <>
struct SpecAction<feature_name> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto name = StringPiece{in.begin(), in.end()};
    sp.curFeature_ = &sp.builder_.feature(name);
    sp.features_[name] = sp.curFeature_;
  }
};

template <>
struct SpecAction<fieldparam> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto name = StringPiece{in.begin(), in.end()};
    auto it = sp.fields_.find(name);
    if (it == sp.fields_.end()) {
      throw p::parse_error("could not find field: " + name.str(), in);
    }
    sp.curFld_ = it->second;
  }
};

template <>
struct SpecAction<fldref_litem> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    if (sp.curFld_ == nullptr) {
      throw p::parse_error(
          "current field was nullptr, spec parser error, report this as a bug!",
          in);
    }
    sp.fieldRefs_.push_back(*sp.curFld_);
    sp.curFld_ = nullptr;
  }
};

template <>
struct SpecAction<ft_codepoint> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFeature_->codepoint(sp.intParam_);
  }
};

template <>
struct SpecAction<ft_codepoint_type> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFeature_->codepointType(sp.intParam_);
  }
};

template <>
struct SpecAction<ft_placeholder> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFeature_->placeholder();
  }
};

template <>
struct SpecAction<ft_num_codepts> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFeature_->numCodepoints(*sp.curFld_);
    sp.curFld_ = nullptr;
  }
};

template <>
struct SpecAction<ft_byte_length> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFeature_->length(*sp.curFld_);
    sp.curFld_ = nullptr;
  }
};

template <>
struct SpecAction<mt_rhs_file> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto filename = sp.stringParam_;
    auto res = sp.fileResourece(filename, in.position());
    sp.stringParam_ = res->data();
  }
};

template <>
struct SpecAction<mt_cond> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto& vals = sp.fieldRefs_;
    auto csvData = sp.stringParam_;
    if (vals.size() == 1 &&
        !(util::contains(csvData, ',') || util::contains(csvData, '\n'))) {
      sp.curFeature_->matchData(vals[0], csvData);
    } else {
      sp.curFeature_->matchAnyRowOfCsv(csvData, vals);
    }
    vals.clear();
  }
};

template <>
struct SpecAction<mt_then_body> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto& vals = sp.fieldRefs_;
    std::vector<dsl::FieldExpressionBldr> transfs;
    for (auto& f : vals) {
      transfs.push_back(sp.fields_[f.name()]->value());
    }
    sp.curFeature_->ifTrue(transfs);
    vals.clear();
  }
};

template <>
struct SpecAction<mt_else_body> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto& vals = sp.fieldRefs_;
    std::vector<dsl::FieldExpressionBldr> transfs;
    for (auto& f : vals) {
      transfs.push_back(sp.fields_[f.name()]->value());
    }
    sp.curFeature_->ifFalse(transfs);
    vals.clear();
  }
};

template <>
struct SpecAction<feature_stmt> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curFeature_ = nullptr;
  }
};

// NGRAMS

template <>
struct SpecAction<fref_item> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto name = StringPiece{in.begin(), in.end()};
    auto it1 = sp.fields_.find(name);
    if (it1 != sp.fields_.end()) {
      sp.featureRefs_.push_back(*it1->second);
      return;
    }
    auto it2 = sp.features_.find(name);
    if (it2 != sp.features_.end()) {
      sp.featureRefs_.push_back(*it2->second);
      return;
    }
    throw p::parse_error(
        "there was no field nor feature with name: " + name.str(), in);
  }
};

template <>
struct SpecAction<fref_list> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    if (sp.featureRefs_.empty()) {
      throw p::parse_error("feature reference list can't be empty",
                           in.position());
    }

    sp.featureRefRefs_.emplace_back(std::move(sp.featureRefs_));
    sp.featureRefs_.clear();
  }
};

template <>
struct SpecAction<ngram_stmt> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto ngramOrder = sp.featureRefRefs_.size();
    switch (ngramOrder) {
      case 1:
        sp.builder_.unigram(sp.featureRefRefs_[0]);
        break;
      case 2:
        sp.builder_.bigram(sp.featureRefRefs_[0], sp.featureRefRefs_[1]);
        break;
      case 3:
        sp.builder_.trigram(sp.featureRefRefs_[0], sp.featureRefRefs_[1],
                            sp.featureRefRefs_[2]);
        break;
      default:
        throw p::parse_error(
            "unsupported ngram order: " + std::to_string(ngramOrder),
            in.position());
    }
    sp.featureRefRefs_.clear();
  }
};

// UNKS

template <>
struct SpecAction<char_type_lit> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    auto name = StringPiece(in.begin(), in.end());
    auto clazz = charClassByName(name);
    if (clazz == chars::CharacterClass::FAMILY_OTHERS) {
      throw p::parse_error("invalid/unsupported character class: " + name.str(),
                           in.position());
    }
    sp.charClassStack_.push_back(clazz);
  }
};

template <>
struct SpecAction<char_type_or> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    if (sp.charClassStack_.size() < 2) {
      throw p::parse_error("internal error: char class stack lt 2 ",
                           in.position());
    }
    auto v1 = sp.charClassStack_.back();
    sp.charClassStack_.pop_back();
    auto& v2 = sp.charClassStack_.back();
    v2 = v1 | v2;
  }
};

template <>
struct SpecAction<unk_ft_out> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curUnk_->writeFeatureTo(sp.featureRefs_[0]);
    sp.featureRefs_.clear();
  }
};

template <>
struct SpecAction<unk_srf_out> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curUnk_->outputTo(sp.fieldRefs_);
    sp.fieldRefs_.clear();
  }
};

template <>
struct SpecAction<unk_hdr> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curUnk_ = &sp.builder_.unk(sp.stringParam_, sp.intParam_);
  }
};

template <>
struct SpecAction<unk_def> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curUnk_ = nullptr;
  }
};

template <>
struct SpecAction<unk_cls_single> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    JPP_DCHECK_GT(sp.charClassStack_.size(), 0);
    sp.curUnk_->single(sp.charClassStack_[0]);
    sp.charClassStack_.clear();
  }
};

template <>
struct SpecAction<unk_cls_chunking> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    JPP_DCHECK_GT(sp.charClassStack_.size(), 0);
    sp.curUnk_->chunking(sp.charClassStack_[0]);
    sp.charClassStack_.clear();
  }
};

template <>
struct SpecAction<unk_cls_onoma> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    JPP_DCHECK_GT(sp.charClassStack_.size(), 0);
    sp.curUnk_->onomatopoeia(sp.charClassStack_[0]);
    sp.charClassStack_.clear();
  }
};

template <>
struct SpecAction<unk_cls_numeric> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    JPP_DCHECK_GT(sp.charClassStack_.size(), 0);
    sp.curUnk_->numeric(sp.charClassStack_[0]);
    sp.charClassStack_.clear();
  }
};

template <>
struct SpecAction<unk_cls_normalize> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.curUnk_->normalize();
  }
};

template <>
struct SpecAction<floatconst> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    sp.floatParam_ = std::stof(std::string(in.begin(), in.end()));
  }
};

template <>
struct SpecAction<train_field> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    JPP_DCHECK_NE(sp.curFld_, nullptr);
    sp.builder_.train().field(*sp.curFld_, sp.floatParam_);
    sp.curFld_ = nullptr;
  }
};

template <>
struct SpecAction<train_gold_unk> {
  template <typename Input>
  static void apply(const Input& in, SpecParserImpl& sp) {
    JPP_DCHECK_EQ(sp.fieldRefs_.size(), 2);
    sp.builder_.train().allowGoldUnkWith(sp.fieldRefs_[1], sp.fieldRefs_[0],
                                         sp.stringParam_);
    sp.fieldRefs_.clear();
  }
};

}  // namespace parser
}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_PARSER_IMPL_H

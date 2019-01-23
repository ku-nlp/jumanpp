//
// Created by Arseny Tolmachev on 2017/02/23.
//

#ifndef JUMANPP_SPEC_DSL_H
#define JUMANPP_SPEC_DSL_H

#include <util/array_slice.h>
#include <vector>
#include "core/spec/spec_types.h"
#include "util/flatset.h"
#include "util/inlined_vector.h"
#include "util/memory.hpp"
#include "util/status.hpp"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace spec {

class SpecCompiler;

namespace parser {
struct SpecParserImpl;
}

namespace dsl {

class DslOpBase {
 public:
  virtual ~DslOpBase() = default;
  virtual Status validate() const = 0;
};

class FieldBuilder;
class FeatureBuilder;
class UnkWordBuilder;
class FeatureCombinator;

enum class TransformType {
  Invalid,
  Value,
  ReplaceString,
  ReplaceInt,
  AppendString
};

struct FieldExpressionBldr {
  StringPiece fieldName;
  TransformType type = TransformType::Invalid;
  StringPiece transformString;
  int transformInt = std::numeric_limits<i32>::min();
};

class FieldReference {
  StringPiece name_;
  FieldReference(StringPiece name) : name_{name} {}

  friend class FieldBuilder;

 public:
  StringPiece name() const { return name_; }
};

class StorageAssigner;

class FieldBuilder : public DslOpBase {
  i32 csvColumn_;
  StringPiece name_;
  FieldType columnType_ = FieldType::Error;
  bool trieIndex_ = false;
  std::string emptyValue_;
  StringPiece stringStorage_;
  std::string fieldSeparator_ = " ";
  std::string kvSeparator_ = ":";
  i32 alignment_ = 0;

  FieldBuilder() {}

 public:
  FieldBuilder(i32 csvColumn_, StringPiece name_)
      : csvColumn_(csvColumn_), name_(name_) {}

  FieldBuilder& strings() {
    columnType_ = FieldType::String;
    return *this;
  }

  FieldBuilder& stringLists() {
    columnType_ = FieldType::StringList;
    return *this;
  }

  FieldBuilder& integers() {
    columnType_ = FieldType::Int;
    return *this;
  }

  FieldBuilder& kvLists() {
    columnType_ = FieldType::StringKVList;
    return *this;
  }

  FieldBuilder& trieIndex() {
    trieIndex_ = true;
    return *this;
  }

  FieldBuilder& emptyValue(StringPiece data) {
    emptyValue_ = data.str();
    return *this;
  }

  FieldBuilder& stringStorage(FieldReference ref) {
    stringStorage_ = ref.name();
    return *this;
  }

  FieldBuilder& listSeparator(StringPiece sep) {
    this->fieldSeparator_ = sep.str();
    return *this;
  }

  FieldBuilder& kvSeparator(StringPiece sep) {
    this->kvSeparator_ = sep.str();
    return *this;
  }

  FieldBuilder& align(i32 power) {
    this->alignment_ = power;
    return *this;
  }

  FieldExpressionBldr value() const {
    return FieldExpressionBldr{name_, TransformType::Value, jumanpp::EMPTY_SP,
                               0};
  }

  operator FieldExpressionBldr() const { return value(); }

  i32 getCsvColumn() const { return csvColumn_; }

  StringPiece name() const { return name_; }

  FieldType getColumnType() const { return columnType_; }

  bool isTrieIndex() const { return trieIndex_; }

  virtual Status validate() const override;

  operator FieldReference() const { return FieldReference{name()}; }

  Status fill(FieldDescriptor* descriptor, StringPiece* stringStorName) const;
};

enum class FeatureType {
  Initial,
  Invalid,
  MatchValue,
  MatchCsv,
  Placeholder,
  ByteLength,
  CodepointSize,
  CodepointType,
  Codepoint
};

class FeatureBuilder : DslOpBase {
  bool handled = false;
  bool notUsed = false;
  StringPiece name_;
  FeatureType type_ = FeatureType::Initial;
  StringPiece matchData_;
  util::memory::ManagedVector<StringPiece> fields_;
  util::memory::ManagedVector<FieldExpressionBldr> trueTransforms_;
  util::memory::ManagedVector<FieldExpressionBldr> falseTransforms_;
  i32 intParam_ = InvalidInt;

  void changeType(FeatureType target) {
    if (type_ == FeatureType::Initial) {
      type_ = target;
    } else {
      type_ = FeatureType::Invalid;
    }
  }

  friend class ModelSpecBuilder;
  friend class spec::SpecCompiler;

 public:
  FeatureBuilder(const FeatureBuilder&) = delete;
  FeatureBuilder(StringPiece name_, util::memory::PoolAlloc* alloc)
      : name_(name_),
        fields_{alloc},
        trueTransforms_{alloc},
        falseTransforms_{alloc} {}

  StringPiece name() const { return name_; }

  FeatureBuilder& placeholder() {
    changeType(FeatureType::Placeholder);
    return *this;
  }

  FeatureBuilder& numCodepoints(FieldBuilder& field) {
    fields_.push_back(field.name());
    changeType(FeatureType::CodepointSize);
    return *this;
  }

  FeatureBuilder& length(FieldBuilder& field) {
    fields_.push_back(field.name());
    changeType(FeatureType::ByteLength);
    return *this;
  }

  FeatureBuilder& matchData(FieldReference field, StringPiece value) {
    fields_.push_back(field.name());
    matchData_ = value;
    changeType(FeatureType::MatchValue);
    return *this;
  }

  FeatureBuilder& matchAnyRowOfCsv(
      StringPiece csv, std::initializer_list<FieldReference> fields) {
    return matchAnyRowOfCsv<decltype(fields)>(csv, fields);
  }

  template <typename C>
  FeatureBuilder& matchAnyRowOfCsv(StringPiece csv, const C& fields) {
    for (auto& fld : fields) {
      fields_.push_back(fld.name());
    }
    matchData_ = csv;
    changeType(FeatureType::MatchCsv);
    return *this;
  }

  FeatureBuilder& ifTrue(
      std::initializer_list<FieldExpressionBldr> transforms) {
    return ifTrue<decltype(transforms)>(transforms);
  }

  template <typename C>
  FeatureBuilder& ifTrue(const C& transforms) {
    for (auto&& o : transforms) {
      trueTransforms_.emplace_back(o);
    }
    return *this;
  }

  FeatureBuilder& ifFalse(
      std::initializer_list<FieldExpressionBldr> transforms) {
    return ifFalse<decltype(transforms)>(transforms);
  }

  template <typename C>
  FeatureBuilder& ifFalse(const C& transforms) {
    for (auto&& o : transforms) {
      falseTransforms_.emplace_back(o);
    }
    return *this;
  }

  FeatureBuilder& codepointType(i32 offset) {
    intParam_ = offset;
    changeType(FeatureType::CodepointType);
    return *this;
  }

  FeatureBuilder& codepoint(i32 offset) {
    intParam_ = offset;
    changeType(FeatureType::Codepoint);
    return *this;
  }

  Status validate() const override;
};

class FeatureRef {
  StringPiece name_;

 public:
  FeatureRef() = default;
  FeatureRef(const FieldBuilder& fld) : name_{fld.name()} {}
  FeatureRef(const FeatureBuilder& ft) : name_{ft.name()} {}
  FeatureRef(const FeatureRef& o) = default;
  StringPiece name() const { return name_; }
};

class FeatureCombinator : public DslOpBase {
  util::memory::ManagedVector<util::memory::ManagedVector<FeatureRef>> data;
  friend class ModelSpecBuilder;
  friend class spec::SpecCompiler;

  template <typename C>
  void append(util::memory::PoolAlloc* alloc, const C& obj) {
    data.emplace_back(alloc);
    auto& v = data.back();
    for (auto& ref : obj) {
      v.push_back(ref);
    }
  }

 public:
  FeatureCombinator(util::memory::PoolAlloc* alloc) : data{alloc} {}
  Status validate() const { return Status::Ok(); }
};

struct UnkProcFeature {
  FeatureRef ref;
  UnkFeatureType type;
};

class UnkProcBuilder : public DslOpBase {
  StringPiece name_;
  UnkMakerType type_ = UnkMakerType::Invalid;
  chars::CharacterClass charClass_ = chars::CharacterClass::FAMILY_OTHERS;
  i32 pattern_ = -1;
  i32 priority_ = 0;
  std::vector<UnkProcFeature> sideFeatures_;
  std::vector<FieldReference> output_;

  friend class ::jumanpp::core::spec::SpecCompiler;

 public:
  UnkProcBuilder(StringPiece name, i32 pattern)
      : name_{name}, pattern_{pattern} {}

  UnkProcBuilder(const UnkProcBuilder&) = delete;

  UnkProcBuilder& single(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Single;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& chunking(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Chunking;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& onomatopoeia(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Onomatopoeia;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& numeric(chars::CharacterClass charClass) {
    type_ = UnkMakerType::Numeric;
    charClass_ = charClass;
    return *this;
  }

  UnkProcBuilder& normalize() {
    type_ = UnkMakerType::Normalize;
    return *this;
  }

  UnkProcBuilder& writeFeatureTo(FeatureRef ref) {
    JPP_DCHECK_NE(type_, UnkMakerType::Invalid);
    UnkFeatureType tpe = UnkFeatureType::NotPrefixOfDic;
    if (type_ == UnkMakerType::Normalize) {
      tpe = UnkFeatureType::NormalizedActions;
    }
    UnkProcFeature upf{ref, tpe};
    sideFeatures_.emplace_back(upf);
    return *this;
  }

  template <typename C>
  UnkProcBuilder& outputTo(const C& fields) {
    for (auto& x : fields) {
      output_.push_back(x);
    }
    return *this;
  }

  UnkProcBuilder& outputTo(std::initializer_list<FieldReference> field) {
    return outputTo<decltype(field)>(field);
  }

  UnkProcBuilder& lowPriority() {
    priority_ = 1;
    return *this;
  }

  StringPiece name() const { return name_; }

  Status validate() const override;
};

struct GoldAlias {
  StringPiece targetField;
  StringPiece sourceField;
  StringPiece key;
};

class TrainExampleSpec {
  std::vector<std::pair<FieldReference, float>> fields;
  std::vector<GoldAlias> alias;
  friend class ::jumanpp::core::spec::SpecCompiler;

 public:
  TrainExampleSpec& field(FieldReference ref, float weight) {
    fields.push_back({ref, weight});
    return *this;
  }

  TrainExampleSpec& allowGoldUnkWith(FieldReference target, FieldReference src,
                                     StringPiece key) {
    alias.push_back({target.name(), src.name(), key});
    return *this;
  }
};

class ModelSpecBuilder : public DslOpBase {
  util::memory::Manager memmgr_;
  std::unique_ptr<util::memory::PoolAlloc> alloc_;
  util::memory::ManagedVector<FieldBuilder*> fields_;
  util::memory::ManagedVector<FeatureBuilder*> features_;
  util::memory::ManagedVector<FeatureCombinator*> ngrams_;
  util::memory::ManagedVector<UnkProcBuilder*> unks_;
  util::memory::ManagedPtr<TrainExampleSpec> train_;
  std::vector<DslOpBase*> garbage_;

  Status checkNoFeatureIsLeft() const;

 public:
  ModelSpecBuilder(size_t page_size = 128 * 1024)
      : memmgr_{page_size},
        alloc_{memmgr_.core()},
        fields_{alloc_.get()},
        features_{alloc_.get()},
        ngrams_{alloc_.get()},
        unks_{alloc_.get()} {}

  ModelSpecBuilder(const ModelSpecBuilder& o) = delete;

  ~ModelSpecBuilder();

  FieldBuilder& field(i32 csvColumn, StringPiece name) {
    auto ptr = alloc_->make<FieldBuilder>(csvColumn, name);
    garbage_.push_back(ptr);
    fields_.emplace_back(ptr);
    return *ptr;
  }

  FeatureBuilder& feature(StringPiece name) {
    auto ptr = alloc_->make<FeatureBuilder>(name, alloc_.get());
    garbage_.push_back(ptr);
    features_.emplace_back(ptr);
    return *ptr;
  }

  UnkProcBuilder& unk(StringPiece name, i32 pattern) {
    auto ptr = alloc_->make<UnkProcBuilder>(name, pattern);
    garbage_.push_back(ptr);
    unks_.emplace_back(ptr);
    return *ptr;
  }

  void unigram(const std::initializer_list<FeatureRef>& t0) {
    unigram<decltype(t0)>(t0);
  }

  template <typename C0>
  void unigram(const C0& t0) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    garbage_.push_back(cmb);
    cmb->append(alloc_.get(), t0);
    ngrams_.emplace_back(cmb);
  }

  void bigram(const std::initializer_list<FeatureRef>& t1,
              const std::initializer_list<FeatureRef>& t0) {
    bigram<decltype(t1), decltype(t0)>(t1, t0);
  }

  template <typename C1, typename C0>
  void bigram(const C1& t1, const C0& t0) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    garbage_.push_back(cmb);
    cmb->append(alloc_.get(), t0);
    cmb->append(alloc_.get(), t1);
    ngrams_.emplace_back(cmb);
  }

  void trigram(const std::initializer_list<FeatureRef>& t2,
               const std::initializer_list<FeatureRef>& t1,
               const std::initializer_list<FeatureRef>& t0) {
    trigram<decltype(t2), decltype(t1), decltype(t0)>(t2, t1, t0);
  }

  template <typename C2, typename C1, typename C0>
  void trigram(const C2& t2, const C1& t1, const C0& t0) {
    auto cmb = alloc_->make<FeatureCombinator>(alloc_.get());
    garbage_.push_back(cmb);
    cmb->append(alloc_.get(), t0);
    cmb->append(alloc_.get(), t1);
    cmb->append(alloc_.get(), t2);
    ngrams_.emplace_back(cmb);
  }

  TrainExampleSpec& train() {
    if (!train_) {
      train_ = alloc_->make_unique<TrainExampleSpec>();
    }
    return *train_;
  }

  Status validateFields() const;
  Status validateNames() const;
  Status validateFeatures() const;
  Status validateUnks() const;
  virtual Status validate() const override;
  Status build(AnalysisSpec* spec);

  friend class ::jumanpp::core::spec::SpecCompiler;
  friend struct ::jumanpp::core::spec::parser::SpecParserImpl;
};
}  // namespace dsl
}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_DSL_H

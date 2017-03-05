//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_TYPES_H
#define JUMANPP_FEATURE_IMPL_TYPES_H

#include "core/analysis/extra_nodes.h"
#include "core/core_types.h"
#include "core/impl/field_reader.h"
#include "util/array_slice.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

constexpr static u64 PatternFeatureSeed = 0x7a11ed00000000ULL;
constexpr static u64 UnigramSeed = 0x5123a31421fULL;
constexpr static u64 BigramSeed = 0x5123a68442fULL;
constexpr static u64 TrigramSeed = 0x51239ab41f1fULL;

class DicListTraversal {
  dic::impl::IntListTraversal trav_;

 public:
  DicListTraversal(const dic::impl::IntListTraversal& trav) : trav_{trav} {}
  bool next(i32* result) { return trav_.readOneCumulative(result); }
};

/**
 * This showsh which dictionary storage field for a column
 * should be used for computations
 */
enum class LengthFieldSource { Invalid, Strings, Positions };

class FeatureConstructionContext {
  const dic::FieldsHolder* fields;

 public:
  FeatureConstructionContext(const dic::FieldsHolder* fields);

  Status checkFieldType(
      i32 field, std::initializer_list<spec::ColumnType> columnTypes) const;

  Status checkProvidedFeature(u32 index) const { return Status::Ok(); }

  Status setLengthField(i32 fieldNum, LengthFieldSource* field) {
    auto fld = fields->at(fieldNum);
    auto type = fld.columnType;
    if (type == spec::ColumnType::StringList) {
      *field = LengthFieldSource::Positions;
    } else if (type == spec::ColumnType::String) {
      *field = LengthFieldSource::Strings;
    } else {
      return Status::InvalidState()
             << "field " << fld.name << " typed " << fld.columnType
             << " can not be used for length calculation";
    }

    return Status::Ok();
  }
};

class PrimitiveFeatureContext {
  analysis::ExtraNodesContext* extraCtx;
  dic::FieldsHolder* fields;

 public:
  DicListTraversal traversal(i32 fieldIdx, i32 fieldPtr) const {
    auto& fld = fields->at(fieldIdx);
    auto trav = fld.postions.listAt(fieldPtr);
    return DicListTraversal{trav};
  }

  i32 providedFeature(EntryPtr entryPtr, u32 index) const {
    auto node = extraCtx->node(entryPtr);
    if (node == nullptr ||
        node->header.type != analysis::ExtraNodeType::Unknown) {
      return 0;
    }
    return extraCtx->placeholderData(entryPtr, index);
  }

  i32 lengthOf(i32 fieldNum, i32 fieldPtr, LengthFieldSource field) {
    if (fieldPtr < 0) {
      return extraCtx->lengthOf(fieldNum, fieldPtr);
    }
    auto fld = fields->at(fieldNum);
    switch (field) {
      case LengthFieldSource::Positions:
        return fld.postions.lengthOf(fieldPtr);
      case LengthFieldSource::Strings:
        return fld.strings.lengthOf(fieldPtr);
      default:
        return -1;
    }
  }
};

class PrimitiveFeatureData {
  util::ArraySlice<EntryPtr> entries_;
  util::ArraySlice<i32> entryData_;
  util::MutableArraySlice<u64> features_;
  i32 index_ = -1;
  size_t entrySize_;
  size_t featureCnt_;

 public:
  PrimitiveFeatureData(const util::ArraySlice<EntryPtr>& entries_,
                       const util::ArraySlice<i32>& entryData_,
                       const util::MutableArraySlice<u64>& features_)
      : entries_(entries_),
        entryData_(entryData_),
        features_(features_),
        entrySize_{entryData_.size() / entries_.size()},
        featureCnt_{features_.size() / entries_.size()} {}

  bool next() {
    ++index_;
    return index_ < entries_.size();
  }

  EntryPtr entry() const { return entries_[index_]; }

  util::ArraySlice<i32> entryData() const {
    return util::ArraySlice<i32>{entryData_, index_ * entrySize_, entrySize_};
  }

  util::MutableArraySlice<u64> featureData() const {
    return util::MutableArraySlice<u64>{features_, index_ * featureCnt_,
                                        featureCnt_};
  }
};

class PatternFeatureData {
  util::ArraySlice<u64> primitive_;
  util::MutableArraySlice<u64> patterns_;
  i32 index_ = -1;
  u64 primitiveSize;
  u64 patternSize;
  u64 total_;

 public:
  PatternFeatureData(const util::ArraySlice<u64>& primitive_,
                     const util::MutableArraySlice<u64>& patterns_,
                     u64 primitiveSize, u64 patternSize)
      : primitive_(primitive_),
        patterns_(patterns_),
        primitiveSize(primitiveSize),
        patternSize(patternSize),
        total_{primitive_.size() / primitiveSize} {}

  bool next() {
    ++index_;
    return index_ < total_;
  }

  util::ArraySlice<u64> primitive() const {
    return util::ArraySlice<u64>{primitive_, index_ * primitiveSize,
                                 primitiveSize};
  }

  util::MutableArraySlice<u64> pattern() const {
    return util::MutableArraySlice<u64>{patterns_, index_ * patternSize,
                                        patternSize};
  }
};

class NgramFeatureData {
  util::ArraySlice<u64> t2;  // beam size
  util::ArraySlice<u64> t1;  // only one
  util::ArraySlice<u64> t0;  // all in lattice boundary
  util::MutableArraySlice<u64> final;
  i32 t2index = -1;
  i32 t0index = -1;
  u64 t2size;
  u64 t0size;
  u64 finalSize;

 public:
  NgramFeatureData(const util::ArraySlice<u64>& t2,
                   const util::ArraySlice<u64>& t1,
                   const util::ArraySlice<u64>& t0,
                   const util::MutableArraySlice<u64>& final)
      : t2(t2),
        t1(t1),
        t0(t0),
        final(final),
        t2size{t2.size() / t1.size()},
        t0size{t0.size() / t1.size()},
        finalSize{final.size() / t0size} {}

  bool nextT2() {
    ++t2index;
    return t2index < t2size;
  }

  bool nextT0() {
    ++t0index;
    return t0index < t0size;
  }

  util::ArraySlice<u64> patternT2() const {
    return util::ArraySlice<u64>{t2, t2index * t2size, t2size};
  }

  util::ArraySlice<u64> patternT1() const { return t1; }

  util::ArraySlice<u64> patternT0() const {
    return util::ArraySlice<u64>{t0, t0index * t0size, t0size};
  }

  util::MutableArraySlice<u64> finalFeatures() const {
    return util::MutableArraySlice<u64>{final, t0index * finalSize, finalSize};
  }
};

}  // impl
}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_IMPL_TYPES_H

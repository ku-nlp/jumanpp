//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_TYPES_H
#define JUMANPP_FEATURE_IMPL_TYPES_H

#include "core/analysis/extra_nodes.h"
#include "core/core_types.h"
#include "core/impl/field_reader.h"
#include "util/array_slice.h"
#include "util/sliceable_array.h"
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
  explicit DicListTraversal(const dic::impl::IntListTraversal& trav)
      : trav_{trav} {}
  bool next(i32* result) { return trav_.readOneCumulative(result); }
};

class KeyValueTraversal {
  dic::impl::KeyValueListTraversal trav_;

 public:
  explicit KeyValueTraversal(
      const dic::impl::KeyValueListTraversal& trav) noexcept
      : trav_{trav} {}
  bool next() noexcept { return trav_.moveNext(); }
  i32 key() const noexcept { return trav_.key(); }
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
  const dic::FieldsHolder& fields;

 public:
  PrimitiveFeatureContext(analysis::ExtraNodesContext* extraCtx,
                          const dic::FieldsHolder& fields)
      : extraCtx(extraCtx), fields(fields) {}

  DicListTraversal traversal(i32 fieldIdx, i32 fieldPtr) const {
    auto& fld = fields.at(fieldIdx);
    auto trav = fld.postions.listAt(fieldPtr);
    return DicListTraversal{trav};
  }

  KeyValueTraversal kvTraversal(i32 fieldIdx, i32 fieldPtr) const {
    auto& fld = fields.at(fieldIdx);
    auto trav = fld.postions.kvListAt(fieldPtr);
    return KeyValueTraversal{trav};
  }

  i32 providedFeature(EntryPtr entryPtr, u32 index) const {
    if (!entryPtr.isSpecial()) {
      return 0;
    }
    auto node = extraCtx->node(entryPtr);
    if (node == nullptr ||
        node->header.type != analysis::ExtraNodeType::Unknown) {
      return 0;
    }
    return extraCtx->placeholderData(entryPtr, index);
  }

  i32 lengthOf(NodeInfo nodeInfo, i32 fieldNum, i32 fieldPtr,
               LengthFieldSource field, bool useBytes) {
    if (fieldPtr < 0) {
      return extraCtx->lengthOf(nodeInfo.entryPtr());
    }
    auto fld = fields.at(fieldNum);
    switch (field) {
      case LengthFieldSource::Positions:
        return fld.postions.lengthOf(fieldPtr);
      case LengthFieldSource::Strings:
        if (useBytes) {
          return fld.strings.lengthOf(fieldPtr);
        } else {
          return fld.strings.numCodepoints(fieldPtr);
        }
      default:
        return -1;
    }
  }
};

class ComputeFeatureContext {};

class PrimitiveFeatureData {
  util::ArraySlice<NodeInfo> entries_;
  util::Sliceable<i32> entryData_;
  mutable util::Sliceable<u64> features_;
  i32 index_ = -1;

 public:
  PrimitiveFeatureData(const util::ArraySlice<NodeInfo>& entries_,
                       const util::Sliceable<i32>& entryData_,
                       const util::Sliceable<u64>& features_)
      : entries_(entries_), entryData_(entryData_), features_(features_) {}

  bool next() {
    ++index_;
    return index_ < entries_.size();
  }

  NodeInfo nodeInfo() const { return entries_[index_]; }

  util::ArraySlice<i32> entryData() const { return entryData_.row(index_); }

  util::MutableArraySlice<u64> featureData() const {
    return features_.row(index_);
  }
};

class PatternFeatureData {
  util::Sliceable<u64> primitive_;
  mutable util::Sliceable<u64> patterns_;
  i32 index_ = -1;

 public:
  PatternFeatureData(const util::Sliceable<u64>& primitive_,
                     const util::Sliceable<u64>& patterns_)
      : primitive_(primitive_), patterns_(patterns_) {}

  bool next() {
    ++index_;
    return index_ < primitive_.numRows();
  }

  util::ArraySlice<u64> primitive() const { return primitive_.row(index_); }

  util::MutableArraySlice<u64> pattern() const { return patterns_.row(index_); }
};

class NgramFeatureData {
  util::ArraySlice<u64> t2;      // beam size
  util::ArraySlice<u64> t1;      // only one
  util::ConstSliceable<u64> t0;  // all in lattice boundary
  mutable util::Sliceable<u32> final;
  i32 t0index = -1;

 public:
  NgramFeatureData(const util::Sliceable<u32>& final,
                   const util::ArraySlice<u64>& t2,
                   const util::ArraySlice<u64>& t1,
                   const util::ConstSliceable<u64>& t0)
      : t2(t2), t1(t1), t0(t0), final(final) {}

  bool nextT0() {
    ++t0index;
    return t0index < t0.numRows();
  }

  util::ArraySlice<u64> patternT2() const { return t2; }

  util::ArraySlice<u64> patternT1() const { return t1; }

  util::ArraySlice<u64> patternT0() const { return t0.row(t0index); }

  util::MutableArraySlice<u32> finalFeatures() const {
    return final.row(t0index);
  }
};

}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_FEATURE_IMPL_TYPES_H

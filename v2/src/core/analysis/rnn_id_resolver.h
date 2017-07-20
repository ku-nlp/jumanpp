//
// Created by Arseny Tolmachev on 2017/05/31.
//

#ifndef JUMANPP_RNN_ID_RESOLVER_H
#define JUMANPP_RNN_ID_RESOLVER_H

#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_types.h"
#include "core/dictionary.h"
#include "util/flatmap.h"
#include "util/memory.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

struct RnnNodeSpan {
  int position;
  int start;
  int length;
  bool normal;
};

struct IdOffset {
  u32 span;
  u32 id;
};

class RnnAtBoundary;

class RnnIdContainer {
  util::memory::ManagedVector<RnnNodeSpan> spans_;
  util::memory::ManagedVector<IdOffset> offsets_;
  util::memory::ManagedVector<i32> ids_;

 public:
  explicit RnnIdContainer(util::memory::ManagedAllocatorCore* alloc)
      : spans_{alloc}, offsets_{alloc}, ids_{alloc} {}

  void reset() {
    spans_.clear();
    spans_.shrink_to_fit();
    offsets_.clear();
    offsets_.shrink_to_fit();
    ids_.clear();
    ids_.shrink_to_fit();
  }

  void alloc(u32 size) {
    offsets_.reserve(size);
    spans_.reserve(size * 4);
    ids_.reserve(size * 20);
  }

  friend class RnnIdAdder;
  friend class RnnAtBoundary;

  RnnAtBoundary atBoundary(i32 boundary);
};

class RnnAtBoundary {
  RnnIdContainer& container_;
  i32 boundary_;

 public:
  RnnAtBoundary(RnnIdContainer& container_, i32 boundary_)
      : container_(container_), boundary_(boundary_) {}

  i32 numSpans() const {
    auto& off0 = container_.offsets_[boundary_];
    auto& off1 = container_.offsets_[boundary_ + 1];
    return off1.span - off0.span;
  }

  RnnNodeSpan span(i32 span) const {
    auto& off0 = container_.offsets_[boundary_];
    auto& off1 = container_.offsets_[boundary_ + 1];
    JPP_DCHECK_IN(span, 0, off1.span - off0.span);
    return container_.spans_[off0.span + span];
  }

  util::ArraySlice<i32> ids() const {
    auto& off0 = container_.offsets_[boundary_];
    auto& off1 = container_.offsets_[boundary_ + 1];
    util::ArraySlice<i32> idSlice{container_.ids_, off0.id, off1.id - off0.id};
    return idSlice;
  }
};

inline RnnAtBoundary RnnIdContainer::atBoundary(i32 boundary) {
  return RnnAtBoundary{*this, boundary};
}

class RnnIdResolver {
  u32 targetIdx_;
  util::FlatMap<i32, u32> intMap_;
  util::FlatMap<StringPiece, u32> strMap_;
  u32 unkId_ = 0;

 public:
  RnnIdResolver() {}

  Status loadFromDic(const dic::DictionaryHolder& dic, StringPiece colName,
                     util::ArraySlice<StringPiece> rnnDic,
                     StringPiece unkToken);
  Status resolveIds(RnnIdContainer* ids, Lattice* lat,
                    const ExtraNodesContext* xtra) const;

  void serializeMaps(util::CodedBuffer* intBuffer,
                     util::CodedBuffer* stringBuffer) const;
  Status loadFromBuffers(StringPiece intBuffer, StringPiece stringBuffer,
                         u32 targetIdx, u32 unkId);

  i32 resolveId(i32 entry, LatticeBoundary* lb, int position,
                const ExtraNodesContext* xtra) const;

  u32 targetIdx() const { return targetIdx_; }
  u32 unkId() const { return unkId_; }
  size_t numInts() const { return intMap_.size(); }
  size_t numStrings() const { return strMap_.size(); }
};

class RnnIdAdder {
  RnnIdContainer* cont_;
  int boundary_;
  int start_;
  int segment_start_;
  int previous_ = 0;

 public:
  RnnIdAdder(RnnIdContainer* cont, i32 boundary)
      : cont_{cont},
        boundary_{boundary},
        start_{(i32)cont->ids_.size()},
        segment_start_{start_} {
    IdOffset offset{(u32)cont_->spans_.size(), (u32)cont_->ids_.size()};
    cont_->offsets_.push_back(offset);
  }

  void push_unk(int length) {
    int myStart = segment_start_ - start_;
    RnnNodeSpan span{boundary_, myStart, length, false};
    cont_->spans_.push_back(span);
    segment_start_ = (int)cont_->ids_.size();
  }

  void maybe_push_span(bool onAdd) {
    if (previous_ == -1) {
      return;
    }
    int end = static_cast<int>(cont_->ids_.size());
    int myStart = segment_start_ - start_;
    auto myEnd = end - segment_start_;
    RnnNodeSpan span{boundary_, myStart, myEnd, true};
    if (onAdd && span.length == 0) {
      return;
    }
    cont_->spans_.push_back(span);
    segment_start_ = end;
  }

  void add(i32 id, i32 length) {
    if (id == -1) {
      maybe_push_span(true);
      cont_->ids_.push_back(id);
      push_unk(length);
      previous_ = -1;
      return;
    }
    cont_->ids_.push_back(id);
    if (previous_ == -1) {
      previous_ = id;
    }
  }

  void finish() { maybe_push_span(false); }
};

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_ID_RESOLVER_H

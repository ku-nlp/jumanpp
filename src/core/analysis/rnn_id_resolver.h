//
// Created by Arseny Tolmachev on 2017/05/31.
//

#ifndef JUMANPP_RNN_ID_RESOLVER_H
#define JUMANPP_RNN_ID_RESOLVER_H

#include "core/analysis/extra_nodes.h"
#include "core/analysis/lattice_types.h"
#include "core/analysis/rnn_scorer.h"
#include "core/dic/darts_trie.h"
#include "core/dic/dictionary.h"
#include "score_processor.h"
#include "util/flatmap.h"
#include "util/memory.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

class RnnReprBuilder {
  util::CodedBuffer buffer_;

 public:
  void reset() { buffer_.reset(); }
  void addInt(i32 value) { buffer_.writeVarint(static_cast<u32>(value)); }
  void addString(StringPiece value) {
    buffer_.writeStringDataWithoutLengthPrefix(value);
    buffer_.writeVarint(1);
  }
  StringPiece repr() const { return buffer_.contents(); }
};

class RnnIdContainer;

class RnnIdResolver {
  std::vector<u32> fields_;
  dic::DoubleArray knownIndex_;
  dic::DoubleArray unkIndex_;
  i32 unkId_ = 0;

 public:
  StringPiece reprOf(RnnReprBuilder* bldr, EntryPtr eptr,
                     util::ArraySlice<i32> features,
                     const ExtraNodesContext* xtra) const;
  Status setState(util::ArraySlice<u32> fields, StringPiece known,
                  StringPiece unknown, i32 unkid);
  Status resolveIdsAtGbeam(RnnIdContainer* ids, Lattice* lat,
                           const ExtraNodesContext* xtra) const;
  Status build(const dic::DictionaryHolder& dic, const RnnInferenceConfig& cfg,
               util::ArraySlice<StringPiece> rnndic);
  util::ArraySlice<u32> targets() const { return fields_; }
  i32 unkId() const { return unkId_; }

  StringPiece knownIndex() const { return knownIndex_.contents(); }

  StringPiece unkIndex() const { return unkIndex_.contents(); }

  friend class RnnIdContainer;
};

struct RnnCoordinate {
  u16 boundary;
  u16 length;
  i32 rnnId;
};

struct RnnCrdHasher {
  size_t operator()(const RnnCoordinate& crd) const noexcept {
    u64 v;
    std::memcpy(&v, &crd, sizeof(u64));
    v ^= 0xfeadbeed1235;
    v *= 0x12312f12aff1ULL;
    v ^= (v >> 33);
    return v;
  }

  bool operator()(const RnnCoordinate& p1, const RnnCoordinate& p2) const
      noexcept {
    return std::tie(p1.boundary, p1.length, p1.rnnId) ==
           std::tie(p2.boundary, p2.length, p2.rnnId);
  }
};

struct ConnPtrHasher {
  size_t operator()(const ConnectionPtr* ptr) const noexcept {
    u64 v;
    std::memcpy(&v, ptr, sizeof(u64));
    v ^= 0xfeadbeed1235;
    v *= 0x12312f12aff1ULL;
    v ^= (v >> 33);
    return v;
  }

  bool operator()(const ConnectionPtr* p1, const ConnectionPtr* p2) const
      noexcept {
    return *p1 == *p2;
  }
};

struct RnnNode {
  i32 id;
  i32 idx;
  i32 boundary;
  i32 length;
  RnnNode* prev;
  RnnNode* nextInBnd;
  u64 hash;
};

struct RnnScorePtr {
  const ConnectionPtr* latPtr;
  RnnScorePtr* next;
  RnnNode* rnn;
};

struct RnnBoundary {
  RnnNode* node = nullptr;
  RnnScorePtr* scores = nullptr;
  i32 nodeCnt = 0;
  i32 scoreCnt = 0;
};

class RnnIdContainer {
  util::memory::PoolAlloc* alloc_;
  util::FlatMap<RnnCoordinate, RnnNode*, RnnCrdHasher, RnnCrdHasher> crdCache_;
  util::FlatMap<const ConnectionPtr*, RnnNode*, ConnPtrHasher, ConnPtrHasher>
      ptrCache_{};
  util::FlatMap<LatticeNodePtr, RnnCoordinate> nodeCache_;
  std::vector<RnnBoundary> boundaries_{};
  RnnReprBuilder reprBldr_;

 private:
  std::pair<RnnNode*, RnnNode*> addPrevChain(const RnnIdResolver* resolver,
                                             const Lattice* lat,
                                             const ConnectionPtr* cptr,
                                             const ExtraNodesContext* xtra);
  void addScore(RnnNode* node, const ConnectionPtr* cptr);
  void addPath(const RnnIdResolver* resolver, const Lattice* lat,
               const ConnectionPtr* cptr, const ExtraNodesContext* xtra);
  const RnnCoordinate& resolveId(const RnnIdResolver* resolver,
                                 const Lattice* lat, const ConnectionPtr* node,
                                 const ExtraNodesContext* xtra);
  void addBos();
  ConnectionPtr* fakeConnection(const LatticeNodePtr& nodePtr,
                                const ConnectionBeamElement* beam,
                                const BeamCandidate& cand);

 public:
  RnnIdContainer(util::memory::PoolAlloc* alloc) noexcept : alloc_{alloc} {}
  RnnIdContainer(const RnnIdContainer&) = delete;
  void reset(u32 numBoundaries, u32 beamSize);
  const RnnBoundary& rnnBoundary(u32 bndIdx) const {
    JPP_DCHECK_IN(bndIdx, 0, boundaries_.size());
    return boundaries_[bndIdx];
  }
  friend class RnnIdResolver;
};

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_RNN_ID_RESOLVER_H

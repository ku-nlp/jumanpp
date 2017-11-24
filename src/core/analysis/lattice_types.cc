//
// Created by Arseny Tolmachev on 2017/02/27.
//

#include "lattice_types.h"
#include "util/stl_util.h"

#ifdef __SSE2__
#include <emmintrin.h>
#endif

namespace jumanpp {
namespace core {
namespace analysis {

Lattice::Lattice(util::memory::PoolAlloc *alloc, const LatticeConfig &lc)
    : boundaries{alloc}, lconf{lc}, alloc{alloc} {}

Status Lattice::makeBoundary(const LatticeBoundaryConfig &lbc,
                             LatticeBoundary **ptr) {
  auto bnd = alloc->make<LatticeBoundary>(alloc, lconf, lbc);
  JPP_RETURN_IF_ERROR(bnd->initialize());
  this->boundaries.push_back(bnd);
  *ptr = bnd;
  return Status::Ok();
}

void Lattice::reset() {
  boundaries.clear();
  boundaries.shrink_to_fit();
}

void Lattice::hintSize(u32 size) { boundaries.reserve(size); }

LatticeBoundary::LatticeBoundary(util::memory::PoolAlloc *alloc,
                                 const LatticeConfig &lc,
                                 const LatticeBoundaryConfig &lbc)
    : cfg_{lbc},
      left_{alloc, lc, cfg_},
      right_{alloc, lc, cfg_},
      scores_{alloc, lc, cfg_},
      currentEnding_{0} {}

Status LatticeBoundary::initialize() {
  return Status::Ok();
}

void LatticeBoundary::addEnd(LatticeNodePtr nodePtr) {
  left_.endingNodes_.at(currentEnding_) = nodePtr;
  ++currentEnding_;
}

LatticeBoundaryScores::LatticeBoundaryScores(util::memory::PoolAlloc *alloc,
                                             const LatticeConfig &lc,
                                             const LatticeBoundaryConfig &lbc)
    : scoresPerItem_{lc.beamSize * lbc.endNodes * lc.scoreCnt},
      numLeft_{lbc.endNodes},
      numBeam_{lc.beamSize},
      numScorers_{lc.scoreCnt} {
  u32 num = lbc.beginNodes;
  // do allocation like this because there can be many elements
  // a single allocation of leftSize * rightSize * beamSize * numScorers can be
  // very large
  auto buffers = alloc->allocateBuf<Score *>(num);
  for (int i = 0; i < num; ++i) {
    buffers[i] = alloc->allocateArray<Score>(scoresPerItem_, 16);
  }
  scores_ = buffers;
}

inline void nonTemporalStore4(void *dest, const void *src) {
#ifdef __SSE2__
  _mm_stream_si32(reinterpret_cast<int*>(dest), *reinterpret_cast<const int*>(src));
#else
  std::memcpy(dest, src, 4);
#endif
}

void LatticeBoundaryScores::importBeamScore(i32 left, i32 scorer, i32 beam,
                                            util::ArraySlice<Score> scores) {
  auto num = scores.size();
  for (int el = 0; el < num; ++el) {
    auto node = nodeScores(el);
    auto svec = node.beamLeft(beam, left);
    static_assert(sizeof(Score) == 4, "");
    //nonTemporalStore4(&svec.at(scorer), &scores.at(i));
    svec.at(scorer) = scores.at(el);
  }
}

LatticeRightBoundary::LatticeRightBoundary(util::memory::PoolAlloc *alloc,
                                           const LatticeConfig &lc,
                                           const LatticeBoundaryConfig &lbc) {
  auto rawNodes = alloc->allocateRawArray<NodeInfo>(lbc.beginNodes, 64);
  nodeInfo_ = util::MutableArraySlice<NodeInfo>{rawNodes, lbc.beginNodes};
  featurePatterns =
      alloc->allocate2d<u64>(lbc.beginNodes, lc.numFeaturePatterns, 64);
  beam =
      alloc->allocate2d<ConnectionBeamElement>(lbc.beginNodes, lc.beamSize, 64);
  entryDataStorage = alloc->allocate2d<i32>(lbc.beginNodes, lc.entrySize, 64);
}

LatticeLeftBoundary::LatticeLeftBoundary(util::memory::PoolAlloc *alloc,
                                         const LatticeConfig &lc,
                                         const LatticeBoundaryConfig &lbc) {
  endingNodes_ = alloc->allocateBuf<LatticeNodePtr>(lbc.endNodes);
  if (lc.globalBeamSize > 0) {
    globalBeam_ =
        alloc->allocateBuf<const ConnectionBeamElement *>(lc.globalBeamSize);
  }
}

void NodeScores::fill(Score v) { util::fill(beamScores_, v); }
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

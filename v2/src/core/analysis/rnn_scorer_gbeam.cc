//
// Created by Arseny Tolmachev on 2017/11/20.
//

#include "rnn_scorer_gbeam.h"
#include "rnn_id_resolver.h"
#include "util/memory.hpp"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

struct GbeamRnnFactoryState {
  rnn::RnnIdResolver2 resolver;
  jumanpp::rnn::mikolov::MikolovRnn rnn;
  util::ConstSliceable<float> embeddings;
  util::ConstSliceable<float> nceEmbeddings;

  u32 embedSize() const { return rnn.modelHeader().layerSize; }

  util::ArraySlice<float> embedOf(size_t idx) const {
    return embeddings.row(idx);
  }

  util::ArraySlice<float> nceEmbedOf(size_t idx) const {
    return nceEmbeddings.row(idx);
  }
};

struct GbeamRnnState {
  const GbeamRnnFactoryState* shared;
  util::memory::Manager manager{4 * 1024 * 1024};
  std::unique_ptr<util::memory::PoolAlloc> alloc{manager.core()};
  rnn::RnnIdContainer2 container{alloc.get()};

  Lattice* lat;
  const ExtraNodesContext* xtra;
  u32 scorerIdx;

  util::Sliceable<i32> ctxIdBuf;
  util::MutableArraySlice<i32> rightIdBuf;
  util::Sliceable<float> contextBuf;
  util::Sliceable<float> embBuf;
  util::MutableArraySlice<float> scoreBuf;
  util::MutableArraySlice<util::Sliceable<float>> contexts;

  void allocateState() {
    auto numBnd = lat->createdBoundaryCount();
    auto gbeamSize = lat->config().globalBeamSize;
    auto embedSize = shared->rnn.modelHeader().layerSize;
    contexts = alloc->allocateBuf<util::Sliceable<float>>(numBnd);
    ctxIdBuf = alloc->allocate2d<i32>(
        gbeamSize, shared->rnn.modelHeader().maxentOrder - 1);
    rightIdBuf = alloc->allocateBuf<i32>(gbeamSize);
    contextBuf = alloc->allocate2d<float>(gbeamSize, embedSize, 64);
    embBuf = alloc->allocate2d<float>(gbeamSize, embedSize, 64);
    scoreBuf = alloc->allocateBuf<float>(gbeamSize, 64);
  }

  util::ArraySlice<i32> gatherIds(const rnn::RnnBoundary& rbnd) {
    size_t nodeCnt = static_cast<size_t>(rbnd.nodeCnt);
    util::MutableArraySlice<i32> subset{rightIdBuf, 0, nodeCnt};
    JPP_INDEBUG(int count = rbnd.nodeCnt);
    for (auto node = rbnd.node; node != nullptr; node = node->nextInBnd) {
      subset.at(node->idx) = node->id;
      JPP_INDEBUG(--count);
    }
    JPP_DCHECK_EQ(count, 0);
    return subset;
  }

  util::ConstSliceable<i32> gatherPrevStateIds(const rnn::RnnBoundary& rbnd) {
    auto subset = ctxIdBuf.topRows(rbnd.nodeCnt);
    auto cnt = ctxIdBuf.rowSize();
    for (auto node = rbnd.node; node != nullptr; node = node->nextInBnd) {
      int idx = 0;
      auto row = subset.row(node->idx);
      auto prev = node->prev;
      for (; idx < cnt; ++idx) {
        JPP_DCHECK_NE(prev, nullptr);
        row.at(idx) = prev->id;
      }
    }
    return subset;
  }

  util::ConstSliceable<float> gatherContext(const rnn::RnnBoundary& rbnd) {
    auto subset = contextBuf.topRows(rbnd.nodeCnt);
    for (auto node = rbnd.node; node != nullptr; node = node->nextInBnd) {
      auto prev = node->prev;
      JPP_DCHECK_NE(prev, nullptr);
      auto ctxRow = subset.row(node->idx);
      auto present = contexts.at(prev->boundary).row(prev->idx);
      util::copy_buffer(present, ctxRow);
    }
    return subset;
  }

  util::ConstSliceable<float> gatherLeftEmbeds(util::ArraySlice<i32> ids) {
    auto subset = embBuf.topRows(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
      auto embedId = ids[i];
      auto embed = shared->embedOf(embedId);
      auto target = subset.row(i);
      util::copy_buffer(embed, target);
    }
    return subset;
  }

  util::ConstSliceable<float> gatherNceEmbeds(util::ArraySlice<i32> ids) {
    auto subset = embBuf.topRows(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
      auto embedId = ids[i];
      auto embed = shared->nceEmbedOf(embedId);
      auto target = subset.row(i);
      util::copy_buffer(embed, target);
    }
    return subset;
  }

  util::Sliceable<float> allocContext(u32 bndIdx, size_t size) {
    auto buf = alloc->allocate2d<float>(size, shared->embedSize(), 64);
    contexts.at(bndIdx) = buf;
    return buf;
  }

  util::MutableArraySlice<float> resizeScores(i32 size) {
    size_t sz = static_cast<size_t>(size);
    util::MutableArraySlice<float> res{scoreBuf, 0, sz};
    return res;
  }

  Status computeContext(u32 bndIdx) {
    auto& rbnd = container.rnnBoundary(bndIdx);
    if (rbnd.nodeCnt == 0) {
      return Status::Ok();
    }

    auto rnnIds = gatherIds(rbnd);
    auto inCtx = gatherContext(rbnd);
    auto embs = gatherLeftEmbeds(rnnIds);
    auto outCtx = allocContext(bndIdx, rnnIds.size());

    jumanpp::rnn::mikolov::ParallelContextData pcd{inCtx, embs, outCtx};
    shared->rnn.computeNewParCtx(&pcd);
    return Status::Ok();
  }

  void copyScoresToLattice(util::MutableArraySlice<float> slice,
                           const rnn::RnnBoundary& boundary, u32 bndIdx) {
    auto bnd = lat->boundary(bndIdx);
    auto scoreStorage = bnd->scores();
    for (auto sc = boundary.scores; sc != nullptr; sc = sc->next) {
      auto ptr = sc->latPtr;
      JPP_DCHECK_EQ(bndIdx, ptr->boundary);
      auto nodeScores = scoreStorage->nodeScores(ptr->right);
      auto score = slice.at(sc->rnn->idx);
      nodeScores.beamLeft(ptr->beam, ptr->left).at(scorerIdx) = score;
    }
  }

  Status scoreBoundary(u32 bndIdx) {
    auto& rbnd = container.rnnBoundary(bndIdx);
    if (rbnd.nodeCnt == 0) {
      return Status::Ok();
    }

    auto rnnIds = gatherIds(rbnd);
    auto scores = resizeScores(rbnd.scoreCnt);

    jumanpp::rnn::mikolov::ParallelStepData psd{
        gatherPrevStateIds(rbnd), rnnIds, gatherContext(rbnd),
        gatherNceEmbeds(rnnIds), scores};

    shared->rnn.applyParallel(&psd);

    copyScoresToLattice(scores, rbnd, bndIdx);

    return Status::Ok();
  }

  Status scoreLattice(Lattice* l, const ExtraNodesContext* xtra) {
    this->lat = l;
    this->xtra = xtra;
    alloc->reset();
    auto numBnd = l->createdBoundaryCount() - 1;
    container.reset(numBnd, l->config().globalBeamSize);
    allocateState();
    JPP_RETURN_IF_ERROR(
        shared->resolver.resolveIdsAtGbeam(&container, l, xtra));
    for (u32 bndIdx = 1; bndIdx < numBnd; ++bndIdx) {
      JPP_RIE_MSG(computeContext(bndIdx), "bnd=" << bndIdx);
    }
    for (u32 bndIdx = 2; bndIdx <= numBnd; ++bndIdx) {
      JPP_RIE_MSG(scoreBoundary(bndIdx), "bnd=" << bndIdx);
    }
    return Status::Ok();
  }
};

RnnScorerGbeam::RnnScorerGbeam() = default;

Status RnnScorerGbeam::scoreLattice(Lattice* l, const ExtraNodesContext* xtra,
                                    u32 scorerIdx) {
  state_->scorerIdx = scorerIdx;
  return state_->scoreLattice(l, xtra);
}

RnnScorerGbeam::~RnnScorerGbeam() = default;

RnnScorerGbeamFactory::RnnScorerGbeamFactory() = default;

Status RnnScorerGbeamFactory::load(const model::ModelInfo& model) {
  return Status::NotImplemented();
}

Status RnnScorerGbeamFactory::makeInstance(
    std::unique_ptr<ScoreComputer>* result) {
  return Status::NotImplemented();
}

RnnScorerGbeamFactory::~RnnScorerGbeamFactory() = default;
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
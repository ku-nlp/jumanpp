//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "rnn_scorer.h"
#include <random>
#include "core/analysis/score_processor.h"
#include "lattice_types.h"
#include "rnn/mikolov_rnn.h"
#include "util/logging.hpp"
#include "util/memory.hpp"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

using namespace jumanpp::rnn::mikolov;

struct RnnHolderState {
  MikolovRnnModelHeader header;
  util::ArraySlice<float> matrix;
  util::Sliceable<float> embeddings;
  util::Sliceable<float> nceEmbeddings;
  util::ArraySlice<float> maxentWeights;
};

RnnHolder::~RnnHolder() {}

RnnScorer::~RnnScorer() {}

struct RnnScorerState {
  util::memory::Manager mgr;
  std::shared_ptr<util::memory::ManagedAllocatorCore> alloc;
  MikolovRnn mrnn;
  const RnnHolderState* rnnState;

  i32 maxRight;
  i32 beamSize;

  // state which is throwed away on each step

  // score is output here
  util::MutableArraySlice<float> scoreBuffer;      // maxRight x beam
  util::MutableArraySlice<float> contextScratch;   // embSize x beam
  util::MutableArraySlice<float> rightEmbScratch;  // embSize x maxRight
  util::MutableArraySlice<i32> contextIds;         // (MaxEntMax x beam)

  // the state, each rightSize
  util::memory::ManagedVector<util::ArraySlice<i32>> entryRnnIds;
  // each: (rows=rightSize x beam) x (cols=embSize)
  util::memory::ManagedVector<util::Sliceable<float>> computedContexts;

  template <typename T>
  util::Sliceable<T> subset(util::MutableArraySlice<T>& b, i32 rows, i32 cols) {
    util::MutableArraySlice<T> sub{b, 0, (u32)rows * cols};
    return util::Sliceable<T>{sub, (u32)cols, (u32)rows};
  }

  void fetchRnnIds(Lattice* l) {
    // BOS
    auto bos = alloc->allocateBuf<i32>(1);
    bos[0] = 0;  // two BOS
    entryRnnIds.push_back(bos);
    entryRnnIds.push_back(bos);

    // TODO actualy fetch ids instead of comping up with some
    std::default_random_engine engine{0xbeef};
    std::uniform_int_distribution<i32> ints{0, (i32)rnnState->header.vocabSize};
    auto bcnt = l->createdBoundaryCount();
    for (int i = 2; i < bcnt - 1; ++i) {
      auto cnt = l->boundary(i)->localNodeCount();
      auto buf = alloc->allocateBuf<i32>(cnt);
      for (int j = 0; j < cnt; ++j) {
        buf.at(j) = ints(engine);
      }
      entryRnnIds.push_back(buf);
    }

    // and EOS
    entryRnnIds.push_back(bos);
  }

  void handleBosNodes(Lattice* l) {
    JPP_DCHECK_GE(l->createdBoundaryCount(), 3);
    auto buf =
        alloc->allocate2d<float>(1, this->rnnState->header.layerSize, 64);
    util::fill(buf, 0);
    computedContexts.push_back(buf);
  }

  void initForLattice(Lattice* l) {
    auto bcnt = l->createdBoundaryCount();
    beamSize = l->config().beamSize;
    auto embSize = rnnState->header.layerSize;
    auto maxEntMax = rnnState->header.maxentOrder;

    handleBosNodes(l);

    maxRight = 0;
    for (int i = 1; i < bcnt; ++i) {
      auto lb = l->boundary(i);
      maxRight = std::max<i32>(maxRight, lb->localNodeCount());
      auto numEnds = beamSize * lb->localNodeCount();
      auto buf = alloc->allocate2d<float>(numEnds, embSize, 64);
      JPP_INDEBUG(util::fill(buf, std::numeric_limits<float>::signaling_NaN()));
      computedContexts.push_back(buf);
    }

    entryRnnIds.reserve(bcnt);
    computedContexts.reserve(bcnt);
    scoreBuffer = alloc->allocateBuf<float>(maxRight * beamSize, 64);
    contextScratch = alloc->allocateBuf<float>(embSize * beamSize, 64);
    rightEmbScratch = alloc->allocateBuf<float>(embSize * maxRight, 64);
    contextIds = alloc->allocateBuf<i32>(maxEntMax * beamSize);

    fetchRnnIds(l);
  }

  void reset() {
    entryRnnIds.clear();
    entryRnnIds.shrink_to_fit();
    computedContexts.clear();
    computedContexts.shrink_to_fit();
    mgr.reset();
    alloc->reset();
  }

  util::Sliceable<float> gatherNceEmbeds(i32 boundary) {
    auto& idxes = entryRnnIds[boundary];
    auto embSize = rnnState->header.layerSize;
    auto storage = subset(rightEmbScratch, idxes.size(), embSize);

    auto nceData = rnnState->nceEmbeddings;

    for (int i = 0; i < idxes.size(); ++i) {
      auto wordId = idxes.at(i);
      auto result = storage.row(i);
      util::copy_buffer(nceData.row(wordId), result);
    }
    return storage;
  }

  i32 calcBeamSize(util::ArraySlice<ConnectionBeamElement> beam) {
    for (int i = 0; i < beam.size(); ++i) {
      if (EntryBeam::isFake(beam.at(i))) {
        return i;
      }
    }
    return beam.size();
  }

  util::Sliceable<i32> gatherContextIds(
      i32 actualBeamSize, util::ArraySlice<ConnectionBeamElement> beamData,
      i32 boundary) {
    // passed size of actual direct context is -1 from the model size
    // the remaining element is the tested node
    auto ctxSize =
        std::min<i32>(rnnState->header.maxentOrder - 1, boundary - 1);
    auto result = subset(contextIds, actualBeamSize, ctxSize);

    for (int i = 0; i < actualBeamSize; ++i) {
      auto& beamElem = beamData.at(i);
      auto perBeamElems = result.row(i);

      const ConnectionPtr* ptr = &beamElem.ptr;
      for (int j = 0; j < ctxSize; ++j) {
        perBeamElems.at(j) = entryRnnIds[ptr->boundary].at(ptr->right);
        ptr = ptr->previous;
      }
    }
    return result;
  }

  util::Sliceable<float> gatherContext(
      i32 actualBeamSize, util::ArraySlice<ConnectionBeamElement> beamData) {
    auto result =
        subset(contextScratch, actualBeamSize, rnnState->header.layerSize);

    for (int i = 0; i < actualBeamSize; ++i) {
      auto& beamElem = beamData.at(i);
      auto ptr = beamElem.ptr.previous;
      auto rowIdx = ptr->beam + beamSize * ptr->right;
      //      LOG_DEBUG() << "rnn state gather: " << i << " " << rowIdx << " {"
      //                  << ptr->boundary << ":" << ptr->right << ":" <<
      //                  ptr->beam
      //                  << "}";
      auto src = computedContexts[ptr->boundary].row(rowIdx);
      auto dest = result.row(i);
      util::copy_buffer(src, dest);
    }

    return result;
  }

  util::ArraySlice<float> embeddingFor(Lattice* l, i32 boundary, i32 leftIdx) {
    auto bnd = l->boundary(boundary);
    auto nodePtr = bnd->ends()->nodePtrs().at(leftIdx);
    auto embIdx = entryRnnIds[nodePtr.boundary].at(nodePtr.position);
    return rnnState->embeddings.row(embIdx);
  }

  util::Sliceable<float> contextOutput(LatticeNodePtr ptr, i32 actualBeamSize) {
    auto thedata = computedContexts[ptr.boundary];
    auto startRow = ptr.position * beamSize;
    auto endRow = startRow + actualBeamSize;
    //    LOG_DEBUG() << "rnn state out [" << ptr.boundary << ":" << startRow <<
    //    ", "
    //                << endRow << "]";
    auto slice = thedata.rows(startRow, endRow);
    return slice;
  }

  util::Sliceable<float> scoreOutput(i32 rightSize, i32 actualBeamSize) {
    return subset(scoreBuffer, actualBeamSize, rightSize);
  }

  void fillScoreData(LatticeBoundary* bnd, i32 leftIdx, i32 scorerIdx,
                     util::Sliceable<float> scoreData) {
    auto conn = bnd->connection(leftIdx);
    for (int i = 0; i < scoreData.numRows(); ++i) {
      conn->importBeamScore(scorerIdx, i, scoreData.row(i));
    }
  }

  bool noNans(util::Sliceable<float> data) const {
    for (auto f : data) {
      if (std::isnan(f)) {
        return false;
      }
    }
    return true;
  }

  void computeScores(Lattice* l, i32 boundary, i32 leftIdx, i32 scorerIdx,
                     util::Sliceable<float> nceData) {
    auto bnd = l->boundary(boundary);
    auto nodePtr = bnd->ends()->nodePtrs().at(leftIdx);
    auto beamData = l->boundary(nodePtr.boundary)
                        ->starts()
                        ->beamData()
                        .row(nodePtr.position);
    auto actualBeamSize = calcBeamSize(beamData);

    auto scores = scoreOutput(bnd->localNodeCount(), actualBeamSize);
    auto ctxVectors = gatherContext(actualBeamSize, beamData);
    StepData args{gatherContextIds(actualBeamSize, beamData, boundary),
                  entryRnnIds[boundary],
                  ctxVectors,
                  embeddingFor(l, boundary, leftIdx),
                  nceData,
                  contextOutput(nodePtr, actualBeamSize),
                  scores};
    mrnn.apply(&args);
    JPP_DCHECK(noNans(args.scores));
    JPP_DCHECK(noNans(args.beamContext));
    fillScoreData(bnd, leftIdx, scorerIdx, scores);
  }

  RnnScorerState()
      : mgr{1024 * 1024},
        alloc{mgr.core()},
        entryRnnIds{alloc.get()},
        computedContexts{alloc.get()} {}

  Status init(const RnnHolder& holder) {
    rnnState = holder.impl_.get();
    return mrnn.init(rnnState->header, rnnState->matrix,
                     rnnState->maxentWeights);
  }
};

Status RnnHolder::init(const MikolovModelReader& model) {
  auto& hdr = model.header();

  auto nceEmbs = model.nceEmbeddings();
  auto nceembSlice = util::Sliceable<float>{
      util::MutableArraySlice<float>{const_cast<float*>(nceEmbs.data()),
                                     nceEmbs.size()},
      hdr.layerSize, hdr.vocabSize};
  auto embs = model.embeddings();
  auto embSlice =
      util::Sliceable<float>{util::MutableArraySlice<float>{
                                 const_cast<float*>(embs.data()), embs.size()},
                             hdr.layerSize, hdr.vocabSize};

  impl_.reset(new RnnHolderState{hdr, model.rnnMatrix(), embSlice, nceembSlice,
                                 model.maxentWeights()});
  return Status::Ok();
}

Status RnnHolder::load(const model::ModelInfo& model) {
  return Status::NotImplemented();
}

Status RnnHolder::makeInstance(std::unique_ptr<ScoreComputer>* result) {
  std::unique_ptr<RnnScorer> ptr{new RnnScorer};
  JPP_RETURN_IF_ERROR(ptr->init(*this));
  result->reset(ptr.release());
  return Status::Ok();
}

RnnHolder::RnnHolder() {}

void RnnScorer::preScore(Lattice* l) {
  state_->reset();
  state_->initForLattice(l);
}

bool RnnScorer::scoreBoundary(i32 scorerIdx, Lattice* l, i32 boundary) {
  auto& s = *state_;
  auto lb = l->boundary(boundary);
  auto endCnt = lb->ends()->arraySize();
  auto rbuf = s.gatherNceEmbeds(boundary);

  for (int leftNodeIdx = 0; leftNodeIdx < endCnt; ++leftNodeIdx) {
    s.computeScores(l, boundary, leftNodeIdx, scorerIdx, rbuf);
  }

  return true;
}

Status RnnScorer::init(const RnnHolder& holder) {
  this->state_.reset(new RnnScorerState);
  return state_->init(holder);
}

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

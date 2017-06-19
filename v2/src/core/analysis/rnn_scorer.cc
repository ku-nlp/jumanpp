//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "rnn_scorer.h"
#include <random>
#include "core/analysis/score_processor.h"
#include "lattice_types.h"
#include "rnn/mikolov_rnn.h"
#include "rnn_id_resolver.h"
#include "util/lazy.h"
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

  RnnIdResolver resolver_;
  RnnInferenceConfig config_;
  float nceConstant_;

  RnnHolderState(const MikolovRnnModelHeader& header,
                 const util::ArraySlice<float>& matrix,
                 const util::Sliceable<float>& embeddings,
                 const util::Sliceable<float>& nceEmbeddings,
                 const util::ArraySlice<float>& maxentWeights)
      : header(header),
        matrix(matrix),
        embeddings(embeddings),
        nceEmbeddings(nceEmbeddings),
        maxentWeights(maxentWeights),
        resolver_{} {}
};

RnnHolder::~RnnHolder() {}

RnnScorer::~RnnScorer() {}

struct RnnScorerState {
  util::memory::Manager mgr;
  std::unique_ptr<util::memory::ManagedAllocatorCore> alloc;
  MikolovRnn mrnn;
  const RnnHolderState* rnnState;

  RnnIdContainer container_;

  i32 maxRight;
  i32 beamSize;

  // state which is throwed away on each step

  // score is output here
  util::MutableArraySlice<float> scoreBuffer;      // maxRight x beam
  util::MutableArraySlice<float> contextScratch;   // embSize x beam
  util::MutableArraySlice<float> rightEmbScratch;  // embSize x maxRight
  util::MutableArraySlice<i32> contextIds;         // (MaxEntMax x beam)

  // the state, each rightSize

  // each: (rows=rightSize x beam) x (cols=embSize)
  util::memory::ManagedVector<util::Sliceable<float>> computedContexts;

  template <typename T>
  util::Sliceable<T> subset(util::MutableArraySlice<T>& b, i32 rows, i32 cols) {
    util::MutableArraySlice<T> sub{b, 0, (u32)rows * cols};
    return util::Sliceable<T>{sub, (u32)cols, (u32)rows};
  }

  Status fetchRnnIds(Lattice* l, ExtraNodesContext* xtra) {
    return rnnState->resolver_.resolveIds(&container_, l, xtra);
  }

  void handleBosNodes(Lattice* l) {
    JPP_DCHECK_GE(l->createdBoundaryCount(), 3);
    auto buf =
        alloc->allocate2d<float>(1, this->rnnState->header.layerSize, 64);
    util::fill(buf, 0);
    computedContexts.push_back(buf);
  }

  Status initForLattice(Lattice* l, ExtraNodesContext* xtra) {
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

    computedContexts.reserve(bcnt);
    scoreBuffer = alloc->allocateBuf<float>(maxRight * beamSize, 64);
    contextScratch = alloc->allocateBuf<float>(embSize * beamSize, 64);
    rightEmbScratch = alloc->allocateBuf<float>(embSize * maxRight, 64);
    contextIds = alloc->allocateBuf<i32>(maxEntMax * beamSize);

    return fetchRnnIds(l, xtra);
  }

  void reset() {
    container_.reset();
    computedContexts.clear();
    computedContexts.shrink_to_fit();
    mgr.reset();
    alloc->reset();
  }

  util::Sliceable<float> gatherNceEmbeds(i32 boundary) {
    auto idxes = container_.atBoundary(boundary).ids();
    auto embSize = rnnState->header.layerSize;
    auto storage = subset(rightEmbScratch, idxes.size(), embSize);

    auto nceData = rnnState->nceEmbeddings;

    for (int i = 0; i < idxes.size(); ++i) {
      auto wordId = idxes.at(i);
      if (wordId < 0) {  // use EOS/BOS markers instead of full unks
        wordId = 0;
      }
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
      auto idxes = container_.atBoundary(ptr->boundary).ids();
      for (int j = 0; j < ctxSize; ++j) {
        perBeamElems.at(j) = idxes.at(ptr->right);
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
    auto embIdx = container_.atBoundary(boundary).ids().at(nodePtr.position);
    if (embIdx >= 0) {
      return rnnState->embeddings.row(embIdx);
    } else {
      return rnnState->embeddings.row(0);
    }
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
    auto idsAtBoundary = container_.atBoundary(boundary);

    auto updatedCtx = contextOutput(nodePtr, actualBeamSize);

    ContextStepData csd{updatedCtx, embeddingFor(l, boundary, leftIdx),
                        ctxVectors};
    mrnn.calcNewContext(csd);
    JPP_DCHECK(noNans(csd.beamContext));

    auto entryData = idsAtBoundary.ids();

    InferStepData isd{gatherContextIds(actualBeamSize, beamData, boundary),
                      entryData, updatedCtx, nceData, scores};

    for (i32 spanNum = 0; spanNum < idsAtBoundary.numSpans(); ++spanNum) {
      auto spanInfo = idsAtBoundary.span(spanNum);
      if (spanInfo.normal) {
        // a span of normal nodes
        mrnn.calcScoresOn(isd, spanInfo.start, spanInfo.length);
      } else {
        // UNK node
        // UNK node spans are guaranteed to have one element

        // Linear cost dependent on length of the item
        // apply it over the whole beam
        for (int i = 0; i < actualBeamSize; ++i) {
          scores.row(i).at(spanInfo.start) =
              rnnState->config_.unkConstantTerm +
              spanInfo.length * rnnState->config_.unkLengthPenalty;
        }
      }
    }

    JPP_DCHECK(noNans(scores));
    fillScoreData(bnd, leftIdx, scorerIdx, scores);
  }

  RnnScorerState()
      : mgr{1024 * 1024},
        alloc{mgr.core()},
        container_{alloc.get()},
        computedContexts{alloc.get()} {}

  Status init(const RnnHolder& holder) {
    rnnState = holder.impl_.get();
    return mrnn.init(rnnState->header, rnnState->matrix,
                     rnnState->maxentWeights);
  }
};

Status RnnHolder::init(const jumanpp::rnn::mikolov::MikolovModelReader& model,
                       const core::dic::DictionaryHolder& dic,
                       StringPiece field) {
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
  JPP_RETURN_IF_ERROR(impl_->resolver_.loadFromDic(dic, field, model.words()));
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

void RnnScorer::preScore(Lattice* l, ExtraNodesContext* xtra) {
  state_->reset();
  state_->initForLattice(l, xtra);
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

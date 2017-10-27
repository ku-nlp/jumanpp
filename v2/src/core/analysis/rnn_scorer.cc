//
// Created by Arseny Tolmachev on 2017/03/11.
//

#include "rnn_scorer.h"
#include <random>
#include "core/analysis/rnn_serialization.h"
#include "core/analysis/score_processor.h"
#include "lattice_types.h"
#include "rnn/mikolov_rnn.h"
#include "rnn_id_resolver.h"
#include "util/array_slice_util.h"
#include "util/lazy.h"
#include "util/logging.hpp"
#include "util/memory.hpp"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

using namespace jumanpp::rnn::mikolov;

struct RnnSerializedState {
  util::CodedBuffer header;
  util::CodedBuffer idsInts;
  util::CodedBuffer idsStrings;
};

struct RnnHolderState {
  MikolovRnnModelHeader header;
  util::ArraySlice<float> matrix;
  util::Sliceable<float> embeddings;
  util::Sliceable<float> nceEmbeddings;
  util::ArraySlice<float> maxentWeights;

  RnnIdResolver resolver_;
  RnnInferenceConfig config_;
  float nceConstant_;

  std::unique_ptr<RnnSerializedState> serializedState_;

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
  std::unique_ptr<util::memory::PoolAlloc> alloc;
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
      if (wordId < 0) {  // we have an unk
        wordId = rnnState->config_.unkId;
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
      for (int j = 0; j < ctxSize; ++j) {
        auto idxes = container_.atBoundary(ptr->boundary).ids();
        perBeamElems.at(j) = idxes.at(ptr->right);
        ptr = ptr->previous;
        JPP_DCHECK(ptr != nullptr);
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
    auto embedIds = container_.atBoundary(nodePtr.boundary).ids();
    auto embIdx = embedIds.at(nodePtr.position);
    if (embIdx >= 0) {
      return rnnState->embeddings.row(embIdx);
    } else {
      return rnnState->embeddings.row(rnnState->config_.unkId);
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
    LatticeBoundaryScores* scores = bnd->scores();
    for (int i = 0; i < scoreData.numRows(); ++i) {
      scores->importBeamScore(leftIdx, scorerIdx, i, scoreData.row(i));
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
    JPP_DCHECK(noNans(csd.curContext));

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
    auto status =
        mrnn.init(rnnState->header, rnnState->matrix, rnnState->maxentWeights);
    mrnn.setNceConstant(holder.impl_->nceConstant_);
    return status;
  }
};

Status RnnHolder::init(const RnnInferenceConfig& conf,
                       const jumanpp::rnn::mikolov::MikolovModelReader& model,
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
  setConfig(conf);
  JPP_RETURN_IF_ERROR(impl_->resolver_.loadFromDic(dic, field, model.words(),
                                                   impl_->config_.unkSymbol));
  impl_->config_.unkId = impl_->resolver_.unkId();
  return Status::Ok();
}

Status RnnHolder::makeInstance(std::unique_ptr<ScoreComputer>* result) {
  std::unique_ptr<RnnScorer> ptr{new RnnScorer};
  JPP_RETURN_IF_ERROR(ptr->init(*this));
  result->reset(ptr.release());
  return Status::Ok();
}

RnnHolder::RnnHolder() {}

Status RnnHolder::makeInfo(model::ModelInfo* result) {
  auto& state = *this->impl_;
  RnnSerializedData serData{state.header, state.config_,
                            state.resolver_.targetIdx()};

  state.serializedState_.reset(new RnnSerializedState);
  auto& savedState = *state.serializedState_;

  util::serialization::Saver headerSaver{&savedState.header};
  headerSaver.save(serData);

  state.resolver_.serializeMaps(&savedState.idsInts, &savedState.idsStrings);
  auto matrix = util::asStringPiece(state.matrix);
  auto embeddings = util::asStringPiece(state.embeddings);
  auto nceEmbeddings = util::asStringPiece(state.nceEmbeddings);
  auto maxentWeights = util::asStringPiece(state.maxentWeights);

  model::ModelPart rnnPart;
  rnnPart.kind = model::ModelPartKind::Rnn;
  rnnPart.data.push_back(savedState.header.contents());
  rnnPart.data.push_back(savedState.idsInts.contents());
  rnnPart.data.push_back(savedState.idsStrings.contents());
  rnnPart.data.push_back(matrix);
  rnnPart.data.push_back(embeddings);
  rnnPart.data.push_back(nceEmbeddings);
  rnnPart.data.push_back(maxentWeights);

  result->parts.push_back(std::move(rnnPart));

  return Status::Ok();
}

template <typename T>
Status toArraySlice(util::ArraySlice<T>* result, StringPiece data,
                    size_t expectedSize) {
  size_t byte_size = data.size();
  size_t numElems = byte_size / sizeof(T);
  if (numElems != expectedSize) {
    return Status::InvalidParameter()
           << "failed to load model part: expected size " << expectedSize
           << " found " << numElems;
  }
  *result =
      util::ArraySlice<T>{reinterpret_cast<const T*>(data.begin()), numElems};
  return Status::Ok();
}

template <typename T>
Status toSliceable(util::Sliceable<T>* result, StringPiece data,
                   size_t expectedRows, size_t expectedCols) {
  size_t byte_size = data.size();
  size_t numElems = byte_size / sizeof(T);
  size_t expectedSize = expectedRows * expectedCols;
  if (numElems != expectedSize) {
    return Status::InvalidParameter()
           << "failed to load model part: expected size " << expectedSize
           << " found " << numElems;
  }
  util::MutableArraySlice<T> slice{
      const_cast<T*>(reinterpret_cast<const T*>(data.begin())), numElems};
  *result = util::Sliceable<T>{slice, expectedCols, expectedRows};
  return Status::Ok();
}

Status RnnHolder::load(const model::ModelInfo& model) {
  auto iter = std::find_if(model.parts.begin(), model.parts.end(),
                           [](const model::ModelPart& mp) {
                             return mp.kind == model::ModelPartKind::Rnn;
                           });
  if (iter == model.parts.end()) {
    return Status::InvalidParameter() << "saved model did not contain rnn";
  }

  const model::ModelPart& rnnPart = *iter;

  if (rnnPart.data.size() != 7) {
    return Status::InvalidParameter()
           << "saved rnn model had " << rnnPart.data.size()
           << " parts, expected 7";
  }

  util::serialization::Loader hdrLoader{rnnPart.data[0]};
  RnnSerializedData headerData;
  if (!hdrLoader.load(&headerData)) {
    return Status::InvalidParameter() << "Rnn Model: failed to load header";
  }

  util::ArraySlice<float> matrix;
  util::Sliceable<float> embeddings;
  util::Sliceable<float> nceEmbeddings;
  util::ArraySlice<float> maxentWeights;
  auto& header = headerData.modelHeader;
  JPP_RETURN_IF_ERROR(toArraySlice<float>(&matrix, rnnPart.data[3],
                                          header.layerSize * header.layerSize));
  JPP_RETURN_IF_ERROR(toSliceable<float>(&embeddings, rnnPart.data[4],
                                         header.vocabSize, header.layerSize));
  JPP_RETURN_IF_ERROR(toSliceable<float>(&nceEmbeddings, rnnPart.data[5],
                                         header.vocabSize, header.layerSize));
  JPP_RETURN_IF_ERROR(
      toArraySlice<float>(&maxentWeights, rnnPart.data[6], header.maxentSize));

  impl_.reset(new RnnHolderState{header, matrix, embeddings, nceEmbeddings,
                                 maxentWeights});

  impl_->config_ = headerData.config;

  JPP_RETURN_IF_ERROR(impl_->resolver_.loadFromBuffers(
      rnnPart.data[1], rnnPart.data[2], headerData.targetIdx_,
      headerData.config.unkId));

  setConfig({});

  return Status::Ok();
}

void RnnHolder::setConfig(const RnnInferenceConfig& conf) {
  RnnInferenceConfig deflt{};
  auto& mycfg = impl_->config_;
  if (conf.nceBias != deflt.nceBias) {
    mycfg.nceBias = conf.nceBias;
  }
  if (conf.unkConstantTerm != deflt.unkConstantTerm) {
    mycfg.unkConstantTerm = conf.unkConstantTerm;
  }
  if (conf.unkLengthPenalty != deflt.unkLengthPenalty) {
    mycfg.unkLengthPenalty = conf.unkLengthPenalty;
  }
  if (conf.perceptronWeight != deflt.perceptronWeight) {
    mycfg.perceptronWeight = conf.perceptronWeight;
  }
  if (conf.rnnWeight != deflt.rnnWeight) {
    mycfg.rnnWeight = conf.rnnWeight;
  }
  if (!conf.unkSymbol.empty()) {
    mycfg.unkSymbol = conf.unkSymbol;
  }
  if (conf.unkId != deflt.unkId) {
    mycfg.unkId = conf.unkId;
  }
  impl_->nceConstant_ = impl_->header.nceLnz + mycfg.nceBias;
}

const RnnInferenceConfig& RnnHolder::config() const { return impl_->config_; }

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
    auto left = lb->ends()->nodePtrs().at(leftNodeIdx);
    if (l->boundary(left.boundary)->isActive()) {
      s.computeScores(l, boundary, leftNodeIdx, scorerIdx, rbuf);
    }
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

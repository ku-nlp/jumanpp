//
// Created by Arseny Tolmachev on 2017/11/20.
//

#include "rnn_scorer_gbeam.h"
#include "rnn/mikolov_rnn.h"
#include "rnn_id_resolver.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {

struct GbeamRnnFactoryState {
  rnn::RnnIdResolver resolver;
  jumanpp::rnn::mikolov::MikolovRnn rnn;
  util::ConstSliceable<float> embeddings;
  util::ConstSliceable<float> nceEmbeddings;
  rnn::RnnInferenceConfig config;
  jumanpp::rnn::mikolov::MikolovModelReader rnnReader;
  util::CodedBuffer codedBuf_;

  u32 embedSize() const { return rnn.modelHeader().layerSize; }

  util::ArraySlice<float> embedOf(size_t idx) const {
    return embeddings.row(idx);
  }

  util::ArraySlice<float> nceEmbedOf(size_t idx) const {
    return nceEmbeddings.row(idx);
  }

  util::memory::Manager mgr{64 * 1024};
  util::Sliceable<float> bosState{};

  void computeBosState(i32 bosId) {
    auto alloc = mgr.core();
    auto zeros = alloc->allocate2d<float>(1, embedSize(), 64);
    util::fill(zeros, 0);
    bosState = alloc->allocate2d<float>(1, embedSize(), 64);
    auto embed = embeddings.row(bosId);
    util::ConstSliceable<float> embedSlice{embed, embedSize(), 1};
    jumanpp::rnn::mikolov::ParallelContextData pcd{zeros, embedSlice, bosState};
    rnn.computeNewParCtx(&pcd);
  }

  StringPiece rnnMatrix() const { return rnn.matrixAsStringpiece(); }

  StringPiece embeddingData() const {
    return StringPiece{
        reinterpret_cast<StringPiece::pointer_t>(embeddings.begin()),
        embeddings.size() * sizeof(float)};
  }

  StringPiece nceEmbedData() const {
    return StringPiece{
        reinterpret_cast<StringPiece::pointer_t>(nceEmbeddings.begin()),
        nceEmbeddings.size() * sizeof(float)};
  }

  StringPiece maxentWeightData() const {
    return rnn.maxentWeightsAsStringpiece();
  }
};

struct GbeamRnnState {
  const GbeamRnnFactoryState* shared;
  util::memory::Manager manager{2 * 1024 * 1024};  // 2M
  std::unique_ptr<util::memory::PoolAlloc> alloc{manager.core()};
  rnn::RnnIdContainer container{alloc.get()};

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
    contexts.at(1) = shared->bosState;
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
      if (embedId == -1) {
        embedId = 0;
      }
      auto embed = shared->embedOf(embedId);
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

  util::ArraySlice<i32> gatherScoreIds(const rnn::RnnBoundary& rbnd) {
    size_t scoreCnt = static_cast<size_t>(rbnd.scoreCnt);
    util::MutableArraySlice<i32> subset{rightIdBuf, 0, scoreCnt};
    u32 scoreIdx = 0;
    for (auto sc = rbnd.scores; sc != nullptr; sc = sc->next) {
      auto node = sc->rnn;
      subset.at(scoreIdx) = node->id;
      scoreIdx += 1;
    }
    JPP_DCHECK_EQ(scoreIdx, scoreCnt);
    return subset;
  }

  util::ConstSliceable<i32> gatherPrevStateIds(const rnn::RnnBoundary& rbnd) {
    auto subset = ctxIdBuf.topRows(rbnd.scoreCnt);
    auto cnt = ctxIdBuf.rowSize();
    u32 scoreIdx = 0;
    for (auto sc = rbnd.scores; sc != nullptr; sc = sc->next) {
      auto node = sc->rnn;
      int idx = 0;
      auto row = subset.row(scoreIdx);
      auto prev = node->prev;
      for (; idx < cnt; ++idx) {
        JPP_DCHECK_NE(prev, nullptr);
        row.at(idx) = prev->id;
      }
      scoreIdx += 1;
    }
    JPP_DCHECK_EQ(scoreIdx, rbnd.scoreCnt);
    return subset;
  }

  util::ConstSliceable<float> gatherScoreContext(const rnn::RnnBoundary& rbnd) {
    auto subset = contextBuf.topRows(rbnd.scoreCnt);
    u32 idx = 0;
    for (auto sc = rbnd.scores; sc != nullptr; sc = sc->next) {
      auto node = sc->rnn;
      auto prev = node->prev;
      JPP_DCHECK_NE(prev, nullptr);
      auto ctxRow = subset.row(idx);
      auto present = contexts.at(prev->boundary).row(prev->idx);
      util::copy_buffer(present, ctxRow);
      idx += 1;
    }
    JPP_DCHECK_EQ(idx, rbnd.scoreCnt);
    return subset;
  }

  util::ConstSliceable<float> gatherNceEmbeds(util::ArraySlice<i32> ids) {
    auto subset = embBuf.topRows(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
      auto embedId = ids[i];
      if (embedId == -1) {
        embedId = 0;
      }
      auto embed = shared->nceEmbedOf(embedId);
      auto target = subset.row(i);
      util::copy_buffer(embed, target);
    }
    return subset;
  }

  util::MutableArraySlice<float> resizeScores(i32 size) {
    size_t sz = static_cast<size_t>(size);
    util::MutableArraySlice<float> res{scoreBuf, 0, sz};
    return res;
  }

  void copyScoresToLattice(util::MutableArraySlice<float> slice,
                           const rnn::RnnBoundary& rbnd, u32 bndIdx) {
    auto bnd = lat->boundary(bndIdx);
    auto scoreStorage = bnd->scores();
    u32 scoreIdx = 0;
    for (auto sc = rbnd.scores; sc != nullptr; sc = sc->next) {
      auto ptr = sc->latPtr;
      JPP_DCHECK_EQ(bndIdx, ptr->boundary);
      auto nodeScores = scoreStorage->nodeScores(ptr->right);
      float score;
      auto rnnId = sc->rnn->id;
      if (rnnId == shared->resolver.unkId()) {
        auto& cfg = shared->config;
        score = cfg.unkConstantTerm + cfg.unkLengthPenalty * sc->rnn->length;
      } else {
        score = slice.at(scoreIdx);
      }
      scoreIdx += 1;
      nodeScores.beamLeft(ptr->beam, ptr->left).at(scorerIdx) = score;
    }
    JPP_DCHECK_EQ(scoreIdx, rbnd.scoreCnt);
  }

  Status scoreBoundary(u32 bndIdx) {
    auto& rbnd = container.rnnBoundary(bndIdx);
    if (rbnd.nodeCnt == 0) {
      return Status::Ok();
    }

    auto rnnIds = gatherScoreIds(rbnd);
    auto scores = resizeScores(rbnd.scoreCnt);

    jumanpp::rnn::mikolov::ParallelStepData psd{
        gatherPrevStateIds(rbnd), rnnIds, gatherScoreContext(rbnd),
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
    allocateState();
    JPP_RETURN_IF_ERROR(
        shared->resolver.resolveIdsAtGbeam(&container, l, xtra));
    for (u32 bndIdx = 2; bndIdx < numBnd; ++bndIdx) {
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

Status RnnScorerGbeamFactory::makeInstance(
    std::unique_ptr<ScoreComputer>* result) {
  auto ptr = new RnnScorerGbeam;
  result->reset(ptr);
  ptr->state_.reset(new GbeamRnnState);
  ptr->state_->shared = state_.get();
  return Status::Ok();
}

Status RnnScorerGbeamFactory::make(StringPiece rnnModelPath,
                                   const dic::DictionaryHolder& dic,
                                   const rnn::RnnInferenceConfig& config) {
  state_.reset(new GbeamRnnFactoryState);
  setConfig(config);
  JPP_RETURN_IF_ERROR(state_->rnnReader.open(rnnModelPath));
  JPP_RETURN_IF_ERROR(state_->rnnReader.parse());
  JPP_RETURN_IF_ERROR(
      state_->resolver.build(dic, state_->config, state_->rnnReader.words()));
  JPP_RETURN_IF_ERROR(state_->rnn.init(state_->rnnReader.header(),
                                       state_->rnnReader.rnnMatrix(),
                                       state_->rnnReader.maxentWeights()));
  auto& h = state_->rnnReader.header();
  state_->embeddings = {state_->rnnReader.embeddings(), h.layerSize,
                        h.vocabSize};
  state_->nceEmbeddings = {state_->rnnReader.nceEmbeddings(), h.layerSize,
                           h.vocabSize};
  if (h.layerSize > 64 * 1024) {
    return JPPS_NOT_IMPLEMENTED << "we don't support embed sizes > 64k";
  }
  state_->computeBosState(0);
  return Status::Ok();
}

const rnn::RnnInferenceConfig& RnnScorerGbeamFactory::config() const {
  return state_->config;
}

void RnnScorerGbeamFactory::setConfig(const rnn::RnnInferenceConfig& config) {
  JPP_DCHECK(state_);
  state_->config.mergeWith(config);
  if (!state_->config.nceBias.isDefault()) {
    state_->rnn.setNceConstant(state_->config.nceBias);
  }
}

struct RnnModelHeader {
  rnn::RnnInferenceConfig& config;
  i32 unkIdx;
  std::vector<u32> fields;
  jumanpp::rnn::mikolov::MikolovRnnModelHeader rnnHeader;
};

template <typename Arch>
void Serialize(Arch& a, RnnModelHeader& o) {
  a& o.config.nceBias;
  a& o.config.unkConstantTerm;
  a& o.config.unkLengthPenalty;
  a& o.config.perceptronWeight;
  a& o.config.rnnWeight;
  a& o.config.eosSymbol;
  a& o.config.unkSymbol;
  a& o.config.rnnFields;
  a& o.config.fieldSeparator;
  a& o.unkIdx;
  a& o.fields;
  a& o.rnnHeader.layerSize;
  a& o.rnnHeader.maxentOrder;
  a& o.rnnHeader.maxentSize;
  a& o.rnnHeader.vocabSize;
  a& o.rnnHeader.nceLnz;
}

Status RnnScorerGbeamFactory::makeInfo(model::ModelInfo* info) {
  if (!state_) {
    return JPPS_INVALID_STATE << "RnnScorerGbeamFactory was not initialized";
  }
  RnnModelHeader header{
      state_->config, state_->resolver.unkId(), {}, state_->rnn.modelHeader()};
  util::copy_insert(state_->resolver.targets(), header.fields);
  util::serialization::Saver s{&state_->codedBuf_};
  s.save(header);

  info->parts.emplace_back();
  auto& part = info->parts.back();
  part.kind = model::ModelPartKind::Rnn;
  part.data.push_back(s.result());
  part.data.push_back(state_->resolver.knownIndex());
  part.data.push_back(state_->resolver.unkIndex());
  part.data.push_back(state_->rnnMatrix());
  part.data.push_back(state_->embeddingData());
  part.data.push_back(state_->nceEmbedData());
  part.data.push_back(state_->maxentWeightData());

  return Status::Ok();
}

Status arr2d(StringPiece data, size_t nrows, size_t ncols,
             util::ConstSliceable<float>* result) {
  if (nrows * ncols * sizeof(float) < data.size()) {
    return JPPS_INVALID_PARAMETER
           << "failed to create ConstSliceable from memory buffer of "
           << data.size() << " need at least " << nrows * ncols * sizeof(float);
  }
  util::ArraySlice<float> wrap{reinterpret_cast<const float*>(data.data()),
                               nrows * ncols};
  *result = util::ConstSliceable<float>{wrap, ncols, nrows};
  return Status::Ok();
}

Status arr1d(StringPiece data, size_t expected,
             util::ArraySlice<float>* result) {
  if (expected * sizeof(float) < data.size()) {
    return JPPS_INVALID_PARAMETER
           << "failed to create ConstSliceable from memory buffer of "
           << data.size() << " need at least " << expected * sizeof(float);
  }
  *result = util::ArraySlice<float>{reinterpret_cast<const float*>(data.data()),
                                    expected};
  return Status::Ok();
}

Status RnnScorerGbeamFactory::load(const model::ModelInfo& model) {
  state_.reset(new GbeamRnnFactoryState);
  auto p = model.firstPartOf(model::ModelPartKind::Rnn);
  if (p == nullptr) {
    return JPPS_INVALID_PARAMETER << "model file did not contain RNN";
  }

  RnnModelHeader header{state_->config, {}, {}, {}};

  util::serialization::Loader l{p->data[0]};
  if (!l.load(&header)) {
    return JPPS_INVALID_PARAMETER << "failed to read RNN header";
  }

  JPP_RETURN_IF_ERROR(state_->resolver.setState(header.fields, p->data[1],
                                                p->data[2], header.unkIdx));
  auto& rnnhdr = header.rnnHeader;

  util::ArraySlice<float> rnnMatrix;
  util::ArraySlice<float> maxentWeights;

  JPP_RIE_MSG(
      arr1d(p->data[3], rnnhdr.layerSize * rnnhdr.vocabSize, &rnnMatrix),
      "failed to read matrix");
  JPP_RIE_MSG(arr2d(p->data[4], rnnhdr.vocabSize, rnnhdr.layerSize,
                    &state_->embeddings),
              "failed to read embeddings");
  JPP_RIE_MSG(arr2d(p->data[5], rnnhdr.vocabSize, rnnhdr.layerSize,
                    &state_->nceEmbeddings),
              "failed to read NCE embeddings");
  JPP_RIE_MSG(arr1d(p->data[6], rnnhdr.maxentSize, &maxentWeights),
              "failed to read NCE embeddings");

  JPP_RETURN_IF_ERROR(state_->rnn.init(rnnhdr, rnnMatrix, maxentWeights));
  state_->computeBosState(0);

  return Status::Ok();
}

RnnScorerGbeamFactory::~RnnScorerGbeamFactory() = default;

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
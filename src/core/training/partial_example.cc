//
// Created by Arseny Tolmachev on 2017/10/07.
//

#include "partial_example.h"
#include "core/analysis/unk_nodes_creator.h"
#include "core/training/loss.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

Status PartialTrainer::prepare() {
  JPP_RETURN_IF_ERROR(analyzer_->resetForInput(example_.surface()));
  JPP_RETURN_IF_ERROR(analyzer_->prepareNodeSeeds());
  JPP_RETURN_IF_ERROR(analyzer_->buildLattice());
  JPP_RETURN_IF_ERROR(analyzer_->bootstrapAnalysis());
  return Status::Ok();
}

Status PartialTrainer::compute(const analysis::ScorerDef* sconf) {
  JPP_RETURN_IF_ERROR(analyzer_->computeScores(sconf));
  JPP_RETURN_IF_ERROR(top1_.fillIn(analyzer_->lattice()));
  features_.clear();
  loss_ = 0;
  handleBoundaryConstraints();
  finalizeFeatures();
  return Status::Ok();
}

void PartialTrainer::handleBoundaryConstraints() {
  auto l = analyzer_->lattice();
  auto eos = l->boundary(l->createdBoundaryCount() - 1);
  auto top1 = eos->starts()->beamData().at(0);
  const analysis::ConnectionPtr* nodeEnd = &top1.ptr;
  auto nodeStart = nodeEnd->previous;
  auto starts = l->boundary(nodeStart->boundary)->starts();
  while (nodeStart->boundary >= 2) {
    auto viol =
        example_.checkViolation(starts, nodeStart->boundary, nodeStart->right);
    switch (viol) {
      case PartialViolation::NoBreak:
      case PartialViolation::Break: {
        addBadNode(nodeStart, nearestValidBnd(nodeStart->boundary));
        addBadNode(nodeStart, nearestValidBnd(nodeEnd->boundary));
        break;
      }
      case PartialViolation::Tag: {
        addBadNode(nodeStart, nodeStart->boundary);
        addBadNode(nodeStart, nodeEnd->boundary);
        break;
      }
      case PartialViolation::None:
        break;
    }
    if (viol != PartialViolation::None) {
      loss_ += 1.0f / top1_.totalNodes();
    }
    nodeEnd = nodeStart;
    nodeStart = nodeStart->previous;
    starts = l->boundary(nodeStart->boundary)->starts();
  }
}

i32 PartialTrainer::nearestValidBnd(i32 boundary) {
  for (; boundary >= 2; --boundary) {
    if (example_.validBoundary(boundary)) {
      return boundary;
    }
  }
  return 2;  // should be always valid
}

void PartialTrainer::finalizeFeatures() {
  for (auto& f : features_) {
    f.feature &= mask_;
  }
  std::sort(std::begin(features_), std::end(features_),
            [](const ScoredFeature& f1, const ScoredFeature& f2) {
              return f1.feature < f2.feature;
            });
  auto nfeatures = features_.size();
  if (nfeatures <= 1) {
    loss_ = 0;
    return;
  }
  int prev = 0;
  for (int cur = 1; cur < nfeatures; ++cur) {
    auto& prevItem = features_[prev];
    auto& curItem = features_[cur];
    if (prevItem.feature == curItem.feature) {
      prevItem.score += curItem.score;
    } else {
      ++prev;
      if (prev != cur) {
        features_[prev] = curItem;
      }
    }
  }
  features_.erase(features_.begin() + prev + 1, features_.end());
  if (features_.size() == 0) {
    loss_ = 0;
  }
}

i32 PartialTrainer::addBadNode(const analysis::ConnectionPtr* node,
                               i32 boundary) {
  auto l = analyzer_->lattice();
  auto goodBnd = l->boundary(boundary);
  auto starts = goodBnd->starts();
  float score = 1.0f / (starts->numEntries() * starts->beamData().rowSize());

  i32 count = 0;

  NgramExampleFeatureCalculator nfc{l, analyzer_->core().features()};

  featureBuf_.resize(analyzer_->core().spec().features.ngram.size());
  util::MutableArraySlice<u32> buffer{&featureBuf_};

  auto beams = starts->beamData();

  for (int i = 0; i < goodBnd->localNodeCount(); ++i) {
    if (!example_.doesNodeMatch(starts, boundary, i)) {
      continue;
    }

    auto beam = beams.row(i);

    for (auto& beamEl : beam) {
      if (analysis::EntryBeam::isFake(beamEl)) {
        break;
      }

      auto t0 = beamEl.ptr;
      auto t1 = t0.previous;
      auto t2 = t1->previous;

      if (!example_.doesNodeMatch(l, t1->boundary, t1->right) ||
          !example_.doesNodeMatch(l, t2->boundary, t2->right)) {
        continue;
      }

      NgramFeatureRef ptrs{t2->latticeNodePtr(), t1->latticeNodePtr(),
                           t0.latticeNodePtr()};

      nfc.calculateNgramFeatures(ptrs, buffer);
      count += 1;

      for (auto f : buffer) {
        features_.push_back(ScoredFeature{f, score});
      }
    }
  }
  if (count == 0) {
    return 0;
  }

  {
    auto t0 = node;
    auto t1 = t0->previous;
    auto t2 = t1->previous;
    // LOG_TRACE() << "Add boundary -features for [" << t0->boundary << "," <<
    // t0->right << "]";
    NgramFeatureRef ref{t2->latticeNodePtr(), t1->latticeNodePtr(),
                        t0->latticeNodePtr()};
    nfc.calculateNgramFeatures(ref, buffer);
    auto negFeature = -count * score;
    for (auto f : buffer) {  // add negative features
      features_.push_back(ScoredFeature{f, negFeature});
    }
  }
  return count;
}

bool PartialExample::doesNodeMatch(const analysis::Lattice* lr, i32 boundary,
                                   i32 position) const {
  auto bndStart = lr->boundary(boundary)->starts();
  return doesNodeMatch(bndStart, boundary, position);
}

bool PartialExample::doesNodeMatch(const analysis::LatticeRightBoundary* lr,
                                   i32 boundary, i32 position) const {
  return checkViolation(lr, boundary, position) == PartialViolation::None;
}

PartialViolation PartialExample::checkViolation(
    const analysis::LatticeRightBoundary* lr, i32 boundary,
    i32 position) const {
  auto len = lr->nodeInfo().at(position).numCodepoints();
  auto end = boundary + len;

  for (auto bnd : noBreak_) {
    // violation on non-breaking
    if (bnd == boundary || bnd == end) {
      return PartialViolation::Break;
    }
    if (bnd > end) {
      break;
    }
  }

  for (auto bnd : boundaries_) {
    if (bnd <= boundary) {
      continue;
    }
    if (bnd >= end) {
      break;
    }
    return PartialViolation::NoBreak;
  }

  auto nodeIter = std::find_if(
      nodes_.begin(), nodes_.end(),
      [boundary](const NodeConstraint& n) { return n.boundary == boundary; });

  if (nodeIter == nodes_.end()) {
    return PartialViolation::None;
  }

  auto& nodeCstrs = *nodeIter;
  if (len != nodeCstrs.length) {
    return PartialViolation::Break;
  }

  auto data = lr->entryData().row(position);
  for (auto& tag : nodeCstrs.tags) {
    if (data.at(tag.field) != tag.value) {
      return PartialViolation::Tag;
    }
  }

  return PartialViolation::None;
}

bool PartialExample::validBoundary(i32 bndIdx) const {
  for (auto chk : noBreak_) {
    if (chk == bndIdx) {
      return false;
    }
    if (chk > bndIdx) {
      break;
    }
  }
  return true;
}

void PartialTrainer::markGold(
    std::function<void(analysis::LatticeNodePtr)> callback,
    analysis::Lattice* l) const {
  for (u16 bnd = 0; bnd < l->createdBoundaryCount(); ++bnd) {
    auto bndobj = l->boundary(bnd);
    auto bndRight = bndobj->starts();
    for (u16 pos = 0; pos < bndRight->numEntries(); ++pos) {
      if (example_.doesNodeMatch(bndRight, bnd, pos)) {
        callback(analysis::LatticeNodePtr{bnd, pos});
      }
    }
  }
}

void PartialTrainer::handleEos() {
  auto l = analyzer_->lattice();
  auto eosBnd = l->createdBoundaryCount() - 1;
  auto eos = l->boundary(eosBnd);
  auto eosBeam = eos->starts()->beamData();
  auto top1 = eosBeam.at(0);

  auto prev = top1.ptr.previous;
  if (example_.doesNodeMatch(l, prev->boundary, prev->right)) {
    return;
  }

  addBadNode(&top1.ptr, eosBnd);
}

Status OwningPartialTrainer::initialize(const TrainerFullConfig& cfg,
                                        const analysis::ScorerDef& scorerDef) {
  analyzer_.initialize(cfg.core, ScoringConfig{cfg.trainingConfig->beamSize, 1},
                       *cfg.analyzerConfig);
  JPP_RETURN_IF_ERROR(analyzer_.value().initScorers(scorerDef));
  u32 numFeatures = 1u << cfg.trainingConfig->featureNumberExponent;
  trainer_.initialize(&analyzer_.value(), numFeatures - 1);
  isPrepared_ = false;
  return Status::Ok();
}

const analysis::OutputManager& OwningPartialTrainer::outputMgr() const {
  return analyzer_.value().output();
}

void OwningPartialTrainer::markGold(
    std::function<void(analysis::LatticeNodePtr)> callback) const {
  trainer_.value().markGold(callback, lattice());
}

analysis::Lattice* OwningPartialTrainer::lattice() const {
  return const_cast<analysis::AnalyzerImpl&>(analyzer_.value()).lattice();
}

void OwningPartialTrainer::setGlobalBeam(const GlobalBeamTrainConfig& cfg) {
  if (analyzer_.value().setGlobalBeam(cfg.leftBeam, cfg.rightCheck,
                                      cfg.rightBeam)) {
    isPrepared_ = false;
    analyzer_.value().reset();
  }
}

Status PartialExampleReader::initialize(TrainingIo* tio,
                                        char32_t noBreakToken) {
  tio_ = tio;
  fields_.clear();
  noBreakToken_ = noBreakToken;
  for (auto& x : tio->fields()) {
    fields_.insert(std::make_pair(x.name, &x));
  }

  return Status::Ok();
}

Status PartialExampleReader::readExample(PartialExample* result, bool* eof) {
  result->file_ = filename_;
  bool firstLine = true;
  i32 boundary = 2;
  result->boundaries_.clear();
  result->surface_.clear();
  result->nodes_.clear();
  while (csv_.nextLine()) {
    if (firstLine) {
      result->line_ = csv_.lineNumber();
      firstLine = false;
    }

    auto wholeLine = csv_.line();
    if (wholeLine.size() > 2 && wholeLine[0] == '#' && wholeLine[1] == ' ') {
      wholeLine.slice(2, wholeLine.size() - 1).assignTo(result->comment_);
      continue;
    }

    if (csv_.numFields() == 1) {
      auto data = csv_.field(0);
      if (data.empty()) {
        auto& bnds = result->boundaries_;
        if (!bnds.empty()) {
          bnds.erase(bnds.end() - 1, bnds.end());
        }
        *eof = false;
        return Status::Ok();
      }
      codepts_.clear();
      JPP_RIE_MSG(chars::preprocessRawData(data, &codepts_),
                  "at " << filename_ << ":" << csv_.lineNumber());

      for (auto& codept : codepts_) {
        if (codept.codepoint == noBreakToken_) {
          result->noBreak_.push_back(boundary);
        } else {
          result->surface_.append(codept.bytes.begin(), codept.bytes.end());
          boundary += 1;
        }
      }

      result->boundaries_.push_back(boundary);
      continue;
    }

    if (!csv_.field(0).empty()) {
      return JPPS_INVALID_PARAMETER
             << "in file: " << filename_ << ":" << csv_.lineNumber()
             << " first field was not empty, but" << csv_.field(0);
    }

    NodeConstraint nc{};

    auto surface = csv_.field(1);
    codepts_.clear();
    JPP_RIE_MSG(chars::preprocessRawData(surface, &codepts_),
                surface << " at " << filename_ << ":" << csv_.lineNumber());
    surface.assignTo(nc.surface);
    nc.length = static_cast<i32>(codepts_.size());
    for (int i = 1; i < nc.length; ++i) {
      result->noBreak_.push_back(boundary + i);
    }
    nc.boundary = boundary;
    boundary += nc.length;
    result->surface_.append(nc.surface);
    result->boundaries_.push_back(boundary);

    for (int idx = 2; idx < csv_.numFields(); ++idx) {
      auto fldData = csv_.field(idx);
      auto it = std::find(fldData.begin(), fldData.end(), ':');
      if (it == fldData.end()) {
        return JPPS_INVALID_PARAMETER
               << "in file: " << filename_ << ":" << csv_.lineNumber()
               << " an entry [" << fldData
               << "] did not contain field name (<name>:<value>)";
      }
      StringPiece fldName = StringPiece{fldData.begin(), it};
      StringPiece fldValue = StringPiece{it + 1, fldData.end()};
      codepts_.clear();
      JPP_RIE_MSG(chars::preprocessRawData(fldValue, &codepts_),
                  fldValue << " at " << filename_ << ":" << csv_.lineNumber());
      auto mapIter = fields_.find(fldName);
      if (mapIter == fields_.end()) {
        return JPPS_INVALID_PARAMETER
               << "in file: " << filename_ << ":" << csv_.lineNumber()
               << " the field name of an entry [" << fldData
               << "] was not present in the dictionary spec";
      }

      auto idmap = mapIter->second->str2int;
      auto idValue = idmap->find(fldValue);
      if (idValue == idmap->end()) {
        auto id = analysis::hashUnkString(fldValue);
        nc.tags.push_back(TagConstraint{mapIter->second->dicFieldIdx, id});
      } else {
        nc.tags.push_back(
            TagConstraint{mapIter->second->dicFieldIdx, idValue->second});
      }
    }

    result->nodes_.push_back(std::move(nc));
  }
  *eof = true;
  return Status::Ok();
}

Status PartialExampleReader::setData(StringPiece data) {
  filename_ = "<memory>";
  return csv_.initFromMemory(data);
}

Status PartialExampleReader::openFile(StringPiece filename) {
  JPP_RETURN_IF_ERROR(file_.open(filename));
  JPP_RETURN_IF_ERROR(setData(file_.contents()));
  filename.assignTo(filename_);
  return Status::Ok();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
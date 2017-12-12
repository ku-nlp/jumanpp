//
// Created by Arseny Tolmachev on 2017/10/07.
//

#include "partial_trainer.h"
#include "core/analysis/unk_nodes_creator.h"
#include "core/impl/feature_computer.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

using core::features::NgramFeaturesComputer;
using core::features::NgramFeatureRef;

Status PartialTrainer::prepare() {
  JPP_RETURN_IF_ERROR(analyzer_->resetForInput(example_.surface()));
  JPP_RETURN_IF_ERROR(analyzer_->prepareNodeSeeds());
  JPP_RETURN_IF_ERROR(analyzer_->buildLattice());
  JPP_RETURN_IF_ERROR(analyzer_->bootstrapAnalysis());
  auto& core = analyzer_->core();
  featureBuf_.resize(core.spec().features.ngram.size());
  tmpBeam_.resize(analyzer_->lattice()->config().beamSize);
  return Status::Ok();
}

Status PartialTrainer::compute(const analysis::ScorerDef* sconf) {
  JPP_RETURN_IF_ERROR(analyzer_->computeScores(sconf));
  JPP_RETURN_IF_ERROR(top1_.fillIn(analyzer_->lattice()));
  features_.clear();
  loss_ = 0;
  memMgr_.reset();
  alloc_->reset();
  handleBoundaryConstraints();
  handleEos();
  finalizeFeatures();
  return Status::Ok();
}

void PartialTrainer::handleBoundaryConstraints() {
  auto l = analyzer_->lattice();
  auto eosBndIdx = l->createdBoundaryCount() - 1;
  auto eos = l->boundary(eosBndIdx);
  auto top1 = eos->starts()->beamData().at(0);
  const analysis::ConnectionPtr* prev = nullptr;
  const analysis::ConnectionPtr* nodeEnd = &top1.ptr;
  auto nodeStart = nodeEnd->previous;
  while (nodeStart->boundary >= 2) {
    auto starts = l->boundary(nodeStart->boundary)->starts();
    auto viol =
        example_.checkViolation(starts, nodeStart->boundary, nodeStart->right);
    switch (viol.kind) {
      case ViolationKind::NoBoundary: {
        auto cands =
            candidatesEndingOn(viol.parameter, viol.parameter, emptyBeam());
        addFeatures(cands.finalize(), nodeStart, nodeEnd, prev);
        break;
      }
      case ViolationKind::WordStart: {
        auto list = findCandidatesSpanning(nodeStart->boundary);
        addFeatures(list.finalize(), nodeStart, nodeEnd, prev);
        break;
      }
      case ViolationKind::WordEnd: {
        auto list = findCandidatesSpanning(nodeEnd->boundary);
        addFeatures(list.finalize(), nodeStart, nodeEnd, prev);
        break;
      }
      case ViolationKind::Tag: {
        auto list = candidatesStartingOn(nodeStart->boundary, emptyBeam());
        addFeatures(list.finalize(), nodeStart, nodeEnd, prev);
        break;
      }
      case ViolationKind::None:
        break;
    }
    if (!viol.isNone()) {
      loss_ += 1.0f / top1_.totalNodes();
    }
    prev = nodeEnd;
    nodeEnd = nodeStart;
    nodeStart = nodeStart->previous;
  }
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
  auto prev2 = prev->previous;
  if (example_.doesNodeMatch(l, prev->boundary, prev->right) &&
      example_.doesNodeMatch(l, prev2->boundary, prev2->right)) {
    return;
  }

  auto eb = emptyBeam();

  for (int i = 1; i < eosBeam.size(); ++i) {
    auto& beam = eosBeam.at(i);
    if (analysis::EntryBeam::isFake(beam)) {
      break;
    }

    auto& t0 = beam.ptr;
    auto& t1 = *t0.previous;
    auto& t2 = *t1.previous;

    // t0 is on EOS and always matches
    if (example_.doesNodeMatch(l, t1.boundary, t1.right) &&
        example_.doesNodeMatch(l, t2.boundary, t2.right)) {
      eb.pushItem(beam);
    }
  }

  auto res = eb.finalize();

  if (res.size() != 0) {
    addFeatures(res, &top1.ptr, nullptr, nullptr);
  }
}

analysis::EntryBeam PartialTrainer::candidatesEndingOn(i32 boundary,
                                                       i32 beforeBoundary,
                                                       analysis::EntryBeam eb) {
  auto lat = analyzer_->lattice();
  auto bndend = lat->boundary(boundary)->ends();
  for (auto ptr : bndend->nodePtrs()) {
    if (ptr.boundary >= beforeBoundary) {
      continue;
    }
    auto bnd = lat->boundary(ptr.boundary);
    auto beams = bnd->starts()->beamData().row(ptr.position);
    for (auto& beam : beams) {
      if (analysis::EntryBeam::isFake(beam)) {
        break;
      }

      if (isValid(beam.ptr)) {
        eb.pushItem(beam);
      }
    }
  }
  return eb;
}

analysis::EntryBeam PartialTrainer::emptyBeam() {
  util::fill(tmpBeam_, analysis::EntryBeam::fake());
  return analysis::EntryBeam{&tmpBeam_};
}

analysis::EntryBeam PartialTrainer::findCandidatesSpanning(i32 boundary) {
  auto eb = emptyBeam();

  auto eosBnd = analyzer_->lattice()->createdBoundaryCount() - 1;

  analysis::ConnectionBeamElement buffer[5];

  u32 valid = 0;

  for (auto bnd = boundary + 1; bnd < eosBnd; ++bnd) {
    analysis::EntryBeam::initializeBlock(buffer);
    auto res = candidatesEndingOn(bnd, boundary, {buffer});
    for (auto& ptr : res.finalize()) {
      ++valid;
      eb.pushItem(ptr);
    }
    if (!eb.isTopFake()) {
      break;
    }
  }

  if (valid == 0) {
    for (auto bnd = boundary + 1; bnd < eosBnd; ++bnd) {
      analysis::EntryBeam::initializeBlock(buffer);
      auto res = makeUpCandidatesEndingOn(bnd, boundary, {buffer});
      for (auto& ptr : res.finalize()) {
        ++valid;
        eb.pushItem(ptr);
      }
      if (valid != 0) {
        break;
      }
    }
  }

  return eb;
}

analysis::EntryBeam PartialTrainer::candidatesStartingOn(
    i32 boundary, analysis::EntryBeam eb) {
  auto lat = analyzer_->lattice();
  auto bnd = lat->boundary(boundary)->starts();

  for (auto& beam : bnd->beamData()) {
    if (analysis::EntryBeam::isFake(beam)) {
      continue;
    }

    if (isValid(beam.ptr)) {
      eb.pushItem(beam);
    }
  }
  return eb;
}

void PartialTrainer::addFeatures(PtrList good,
                                 const analysis::ConnectionPtr* bad,
                                 const analysis::ConnectionPtr* badPrev,
                                 const analysis::ConnectionPtr* badPrev2) {
  goodFeatures_.clear_no_resize();
  badFeatures_.clear_no_resize();

  NgramFeaturesComputer nfc{analyzer_->lattice(), analyzer_->core().features()};
  for (auto& node : good) {
    auto& t0 = node.ptr;
    auto& t1 = *t0.previous;
    auto& t2 = *t1.previous;
    NgramFeatureRef nfr{t2.latticeNodePtr(), t1.latticeNodePtr(),
                        t0.latticeNodePtr()};
    nfc.calculateNgramFeatures(nfr, &featureBuf_);
    for (auto f : featureBuf_) {
      goodFeatures_.insert(f);
    }
  }

  if (goodFeatures_.empty()) {
    return;
  }

  if (!good.empty()) {
    addBiTriCorrections(&good.at(0).ptr, bad, badPrev, badPrev2);
  }

  {
    auto& t0 = *bad;
    auto& t1 = *t0.previous;
    auto& t2 = *t1.previous;
    NgramFeatureRef nfr{t2.latticeNodePtr(), t1.latticeNodePtr(),
                        t0.latticeNodePtr()};
    nfc.calculateNgramFeatures(nfr, &featureBuf_);
    for (auto f : featureBuf_) {
      badFeatures_.insert(f);
    }
  }

  u32 numBad = 0;
  for (auto& f : badFeatures_) {
    if (!goodFeatures_.contains(f)) {
      features_.push_back({f, -1.0f});
      numBad += 1;
    }
  }

  u32 numGood = 0;
  for (auto& f : goodFeatures_) {
    if (!badFeatures_.contains(f)) {
      numGood += 1;
    }
  }

  float goodWeight = 0;
  if (numGood != 0) {
    goodWeight = static_cast<float>(numBad) / numGood;
  }

  for (auto& f : goodFeatures_) {
    if (!badFeatures_.contains(f)) {
      features_.push_back({f, goodWeight});
    }
  }
}

bool PartialTrainer::isValid(const analysis::ConnectionPtr& t0) const {
  auto& t1 = *t0.previous;
  auto& t2 = *t1.previous;
  auto l = analyzer_->lattice();
  return example_.doesNodeMatch(l, t0.boundary, t0.right) &&
         example_.doesNodeMatch(l, t1.boundary, t1.right) &&
         example_.doesNodeMatch(l, t2.boundary, t2.right);
}

analysis::EntryBeam PartialTrainer::makeUpCandidatesEndingOn(
    i32 boundary, i32 beforeBoundary, analysis::EntryBeam eb) {
  auto lat = analyzer_->lattice();
  auto b0end = lat->boundary(boundary)->ends()->nodePtrs();

  for (auto& t0 : b0end) {
    if (t0.boundary >= beforeBoundary) {
      continue;
    }

    if (!example_.doesNodeMatch(lat, t0.boundary, t0.position)) {
      continue;
    }

    auto b1end = lat->boundary(t0.boundary)->ends()->nodePtrs();
    for (auto& t1 : b1end) {
      if (!example_.doesNodeMatch(lat, t1.boundary, t1.position)) {
        continue;
      }

      u32 num = 0;
      auto beam =
          lat->boundary(t1.boundary)->starts()->beamData().row(t1.position);
      for (auto& be : beam) {
        if (analysis::EntryBeam::isFake(be)) {
          break;
        }

        auto t2 = be.ptr.previous;
        if (!example_.doesNodeMatch(lat, t2->boundary, t2->right)) {
          continue;
        }

        auto el = alloc_->allocate<analysis::ConnectionBeamElement>();
        u16 fake = ~u16{0};
        el->ptr = {t0.boundary, fake, t0.position, fake, &be.ptr};
        el->totalScore = be.totalScore + -1000.f;
        eb.pushItem(*el);
        num += 1;
      }
    }
  }

  return eb;
}

void PartialTrainer::addBiTriCorrections(
    const analysis::ConnectionPtr* good, const analysis::ConnectionPtr* bad,
    const analysis::ConnectionPtr* badPrev,
    const analysis::ConnectionPtr* badPrev2) {
  auto lat = analyzer_->lattice();
  if (badPrev == nullptr ||
      !example_.doesNodeMatch(lat, badPrev->boundary, badPrev->right)) {
    return;
  }

  auto ends = lat->boundary(badPrev->boundary)->ends()->nodePtrs();

  auto it = std::find_if(
      ends.begin(), ends.end(), [good](const analysis::LatticeNodePtr& p) {
        return p.boundary == good->boundary && p.position == good->right;
      });

  if (it == ends.end()) {
    return;
  }

  NgramFeaturesComputer nfc{analyzer_->lattice(), analyzer_->core().features()};

  {
    NgramFeatureRef goodNfrBiTri{good->previous->latticeNodePtr(),
                                 good->latticeNodePtr(),
                                 badPrev->latticeNodePtr()};

    nfc.calculateNgramFeatures(goodNfrBiTri, &featureBuf_);
    for (auto f : featureBuf_) {
      goodFeatures_.insert(f);
    }
  }

  {
    NgramFeatureRef badNfrBiTri{bad->previous->latticeNodePtr(),
                                bad->latticeNodePtr(),
                                badPrev->latticeNodePtr()};

    nfc.calculateNgramFeatures(badNfrBiTri, &featureBuf_);
    for (auto f : featureBuf_) {
      badFeatures_.insert(f);
    }
  }

  if (badPrev2 == nullptr ||
      !example_.doesNodeMatch(lat, badPrev2->boundary, badPrev2->right)) {
    return;
  }

  {
    NgramFeatureRef goodNfrTri{good->latticeNodePtr(),
                               badPrev->latticeNodePtr(),
                               badPrev2->latticeNodePtr()};

    nfc.calculateNgramFeatures(goodNfrTri, &featureBuf_);
    for (auto& f : featureBuf_) {
      goodFeatures_.insert(f);
    }
  }

  {
    NgramFeatureRef badNfrTri{bad->latticeNodePtr(), badPrev->latticeNodePtr(),
                              badPrev2->latticeNodePtr()};

    nfc.calculateNgramFeatures(badNfrTri, &featureBuf_);
    for (auto& f : featureBuf_) {
      badFeatures_.insert(f);
    }
  }
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
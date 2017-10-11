//
// Created by Arseny Tolmachev on 2017/10/07.
//

#include "partial_example.h"
#include "core/analysis/unk_nodes_creator.h"
#include "core/training/loss.h"

namespace jumanpp {
namespace core {
namespace training {

Status PartialTrainer::prepare() {
  JPP_RETURN_IF_ERROR(analyzer_->resetForInput(example_.surface()));
  JPP_RETURN_IF_ERROR(analyzer_->prepareNodeSeeds());
  JPP_RETURN_IF_ERROR(analyzer_->buildLattice());
  analyzer_->bootstrapAnalysis();
  return Status::Ok();
}

Status PartialTrainer::compute(const analysis::ScorerDef* sconf) {
  JPP_RETURN_IF_ERROR(analyzer_->computeScores(sconf));
  JPP_RETURN_IF_ERROR(top1_.fillIn(analyzer_->lattice()));
  features_.clear();
  loss_ = 0;
  handleBoundaryConstraints();
  handleTagConstraints();
  finalizeFeatures();
  return Status::Ok();
}

void PartialTrainer::handleBoundaryConstraints() {
  auto l = analyzer_->lattice();
  auto eos = l->boundary(l->createdBoundaryCount() - 1);
  auto top1 = eos->starts()->beamData().at(0);
  auto nodeEnd = top1.ptr.previous;
  auto nodeStart = nodeEnd->previous;
  auto bnditer = std::crbegin(example_.boundaries());
  auto end = std::crend(example_.boundaries());
  while (nodeStart->boundary > 1 && bnditer != end) {
    auto bndary = *bnditer;
    if (nodeStart->boundary == bndary) {
      // Boundaries match, GOOD!
      ++bnditer;
      nodeEnd = nodeStart;
      nodeStart = nodeEnd->previous;
    } else if (nodeStart->boundary < bndary && bndary < nodeEnd->boundary) {
      // BAD: boundary constraint is violated
      addBadNode(nodeStart, bndary);
      loss_ += 1.0f / top1_.totalNodes();
      ++bnditer;
    } else if (bndary >= nodeEnd->boundary) {
      // boundary is after node, move it
      ++bnditer;
    } else {
      // boundary is before node, move node
      nodeEnd = nodeStart;
      nodeStart = nodeEnd->previous;
    }
  }
}

void PartialTrainer::handleTagConstraints() {
  auto l = analyzer_->lattice();
  top1_.reset();
  float nodeRatio = 1.0f / top1_.totalNodes();
  for (auto& nodeConstraint : example_.nodes()) {
    if (!top1_.moveToBoundary(nodeConstraint.boundary)) {
      // There was nothing here
      // Will be handled by boundary constraints
      continue;
    }
    analysis::ConnectionPtr ptr;
    while (top1_.nextNode(&ptr)) {
      auto bnd = l->boundary(ptr.boundary);
      auto& info = bnd->starts()->nodeInfo().at(ptr.right);
      if (info.numCodepoints() != nodeConstraint.length) {
        // Length is incorrect
        loss_ +=
            nodeRatio * addBadNode2(&ptr, ptr.boundary, nodeConstraint.length,
                                    nodeConstraint.tags);
        continue;
      }

      auto entryData = bnd->starts()->entryData().row(ptr.right);

      for (auto& tag : nodeConstraint.tags) {
        if (entryData[tag.field] != tag.value) {
          // We have bad node here!
          loss_ +=
              nodeRatio * addBadNode2(&ptr, ptr.boundary, nodeConstraint.length,
                                      nodeConstraint.tags);
          break;
        }
      }
    }
  }
}

void PartialTrainer::finalizeFeatures() {
  std::sort(std::begin(features_), std::end(features_),
            [](const ScoredFeature& f1, const ScoredFeature& f2) {
              return f1.feature < f2.feature;
            });
  auto nfeatures = features_.size();
  if (nfeatures <= 1) {
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
}

void PartialTrainer::addBadNode(const analysis::ConnectionPtr* node,
                                i32 boundary) {
  auto l = analyzer_->lattice();
  auto goodBnd = l->boundary(boundary);
  auto endingNodes = goodBnd->ends()->nodePtrs();
  float score =
      1.0f / (endingNodes.size() * goodBnd->starts()->beamData().rowSize());

  i32 count = 0;

  NgramExampleFeatureCalculator nfc{l, analyzer_->core().features()};

  featureBuf_.resize(analyzer_->core().runtime().features.ngrams.size());
  util::MutableArraySlice<u32> buffer{&featureBuf_};

  for (auto& end : endingNodes) {  // positive features
    auto bnd = l->boundary(end.boundary);
    auto beam = bnd->starts()->beamData().row(end.position);

    for (auto& beamEl : beam) {
      if (analysis::EntryBeam::isFake(beamEl)) {
        continue;
      }

      if (beamEl.ptr == *node) {
        continue;
      }

      auto t0 = beamEl.ptr;
      auto t1 = t0.previous;
      auto t2 = t1->previous;

      NgramFeatureRef ptrs{t2->latticeNodePtr(), t1->latticeNodePtr(),
                           t0.latticeNodePtr()};

      nfc.calculateNgramFeatures(ptrs, buffer);
      count += 1;

      for (auto f : buffer) {
        features_.push_back(ScoredFeature{f, score});
      }
    }
  }

  {
    auto t0 = node;
    auto t1 = t0->previous;
    auto t2 = t1->previous;
    NgramFeatureRef ref{t2->latticeNodePtr(), t1->latticeNodePtr(),
                        t0->latticeNodePtr()};
    nfc.calculateNgramFeatures(ref, buffer);
    auto negFeature = -count * score;
    for (auto f : buffer) {  // add negative features
      features_.push_back(ScoredFeature{f, negFeature});
    }
  }
}

float PartialTrainer::addBadNode2(const analysis::ConnectionPtr* node,
                                  i32 boundary, i32 length,
                                  util::ArraySlice<TagConstraint> tagFilter) {
  auto l = analyzer_->lattice();
  auto goodBnd = l->boundary(boundary);
  auto bndNodes = goodBnd->starts();

  auto checkTags = [&](int pos) {
    auto entries = bndNodes->entryData().row(pos);
    for (auto& tag : tagFilter) {
      if (entries.at(tag.field) != tag.value) {
        return false;
      }
    }
    return true;
  };

  i32 count = 0;
  i32 nodes = 0;

  // LOOP1: count good nodes
  for (int i = 0; i < bndNodes->arraySize(); ++i) {
    if (bndNodes->nodeInfo().at(i).numCodepoints() != length) {
      continue;
    }

    if (!checkTags(i)) {
      continue;
    }

    auto beam = bndNodes->beamData().row(i);
    for (auto& beamEl : beam) {
      if (analysis::EntryBeam::isFake(beamEl)) {
        continue;
      }
      if (beamEl.ptr == *node) {
        continue;
      }
      count += 1;
    }
    nodes += 1;
  }

  if (count == 0) {
    // do anything if there are no good nodes
    return 0;
  }

  float score = 1.0f / count;

  NgramExampleFeatureCalculator nfc{l, analyzer_->core().features()};

  featureBuf_.resize(analyzer_->core().runtime().features.ngrams.size());
  util::MutableArraySlice<u32> buffer{&featureBuf_};

  // PASS2: compute positive features
  for (int i = 0; i < bndNodes->arraySize(); ++i) {
    if (bndNodes->nodeInfo().at(i).numCodepoints() != length) {
      continue;
    }

    if (!checkTags(i)) {
      continue;
    }

    auto beam = bndNodes->beamData().row(i);
    for (auto& beamEl : beam) {
      if (analysis::EntryBeam::isFake(beamEl)) {
        continue;
      }
      if (beamEl.ptr == *node) {
        continue;
      }
      auto t0 = beamEl.ptr;
      auto t1 = t0.previous;
      auto t2 = t1->previous;

      NgramFeatureRef ptrs{t2->latticeNodePtr(), t1->latticeNodePtr(),
                           t0.latticeNodePtr()};

      nfc.calculateNgramFeatures(ptrs, buffer);
      for (auto f : buffer) {
        features_.push_back(ScoredFeature{f, score});
      }
    }
  }

  {
    auto t0 = node;
    auto t1 = t0->previous;
    auto t2 = t1->previous;
    NgramFeatureRef ref{t2->latticeNodePtr(), t1->latticeNodePtr(),
                        t0->latticeNodePtr()};
    nfc.calculateNgramFeatures(ref, buffer);
    for (auto f : buffer) {  // add negative features
      features_.push_back(ScoredFeature{f, -1});
    }
  }

  return static_cast<float>(nodes) / bndNodes->arraySize();
}

void PartialTrainer::markGold(
    std::function<void(analysis::LatticeNodePtr)> callback,
    analysis::Lattice* l) const {
  auto& ex = example_;
  for (auto bnd : ex.boundaries()) {
    if (std::find_if(ex.nodes().begin(), ex.nodes().end(),
                     [bnd](const NodeConstraint& n) {
                       return n.boundary == bnd;
                     }) != ex.nodes().end()) {
      auto bobj = l->boundary(bnd);
      auto cnt = bobj->starts()->arraySize();
      u16 bndU16 = static_cast<u16>(bnd);
      for (u16 i = 0; i < cnt; ++i) {
        callback(analysis::LatticeNodePtr{bndU16, i});
      }
    }
  }

  for (auto& node : ex.nodes()) {
    auto bnd = l->boundary(node.boundary);
    auto starts = bnd->starts();
    for (int i = 0; i < starts->arraySize(); ++i) {
      auto vals = starts->entryData().row(i);
      for (auto& tag : node.tags) {
        if (vals.at(tag.field) != tag.value) {
          break;
        }
        callback(analysis::LatticeNodePtr{static_cast<u16>(node.boundary),
                                          static_cast<u16>(i)});
      }
    }
  }
}

Status OwningPartialTrainer::initialize(const TrainerFullConfig& cfg,
                                        const analysis::ScorerDef& scorerDef) {
  analyzer_.initialize(&cfg.core, ScoringConfig{1, cfg.trainingConfig.beamSize},
                       cfg.analyzerConfig);
  JPP_RETURN_IF_ERROR(analyzer_.value().initScorers(scorerDef));
  trainer_.initialize(&analyzer_.value());
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

Status PartialExampleReader::initialize(TrainingIo* tio) {
  tio_ = tio;
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
      if (csv_.numFields() == 1) {
        auto fld = csv_.field(0);
        if (fld.size() > 2 && fld[0] == '#' && fld[1] == ' ') {
          fld.from(2).assignTo(result->comment_);
          continue;
        }
      }
    }
    if (csv_.numFields() == 1) {
      auto data = csv_.field(0);
      if (data.empty()) {
        auto& bnds = result->boundaries_;
        if (!bnds.empty()) {
          bnds.erase(bnds.end() - 1, bnds.end());
        }
        return Status::Ok();
      }
      codepts_.clear();
      JPP_RIE_MSG(chars::preprocessRawData(data, &codepts_),
                  "at " << filename_ << ":" << csv_.lineNumber());
      result->surface_.append(data.begin(), data.size());
      boundary += codepts_.size();
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
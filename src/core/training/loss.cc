//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "loss.h"

#include <numeric>
#include "core/analysis/unk_nodes_creator.h"
#include "core/impl/feature_computer.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace training {

using core::features::NgramFeatureRef;
using core::features::NgramFeaturesComputer;

bool LossCalculator::findWorstTopNode(i32 goldPos, ComparisonStep* step) {
  auto lattice = analyzer->lattice();
  auto bndIdx = top1.currentBoundary();
  auto boundary = lattice->boundary(bndIdx);
  auto entries = boundary->starts()->entryData();
  auto goldData = entries.row(goldPos);
  auto& specFlds = spec->fields;

  i32 maxNumMismatches = -1;
  float maxMismatchWeight = -1;
  i32 worstPosition = -1;
  const analysis::ConnectionBeamElement* worstPtr = nullptr;

  // yep, this is assignment
  for (auto ptr = top1.nextBeamPtr(); ptr != nullptr;
       ptr = top1.nextBeamPtr()) {
    auto& conPtr = ptr->ptr;
    auto topData = entries.row(conPtr.right);

    i32 numMismatches = 0;
    float mismatchWeight = 0;

    for (auto& sf : specFlds) {
      if (topData[sf.fieldIdx] != goldData[sf.fieldIdx]) {
        numMismatches += 1;
        mismatchWeight += sf.weight;
      }
    }

    if (maxMismatchWeight < mismatchWeight) {
      maxMismatchWeight = mismatchWeight;
      maxNumMismatches = numMismatches;
      worstPosition = conPtr.right;
      worstPtr = ptr;
    }
  }
  if (worstPosition == -1) {
    return false;
  }

  step->boundary = bndIdx;
  step->topPtr = worstPtr;
  step->goldPosition = goldPos;
  step->numMismatches = maxNumMismatches;
  step->mismatchWeight = maxMismatchWeight;
  return true;
}

Status LossCalculator::computeComparison() {
  comparison.clear();
  comparison.reserve(top1.numBoundaries() + gold.nodes().size() + 1);
  JPP_DCHECK_EQ(goldScores.size(), gold.nodes().size() + 1);
  float goldScore = 0;
  bool hasNextTop = top1.nextBoundary();
  int curGold = 0;
  auto goldPtrs = gold.nodes();
  auto totalGold = goldPtrs.size();
  auto lattice = analyzer->lattice();
  u16 eosPos = static_cast<u16>(lattice->createdBoundaryCount() - 1);
  while (hasNextTop || curGold < totalGold) {
    analysis::LatticeNodePtr curGoldPtr;
    if (curGold < totalGold) {
      curGoldPtr = goldPtrs[curGold];
    } else {
      curGoldPtr = {eosPos, 0};
    }

    i32 curTopBnd = hasNextTop ? top1.currentBoundary() : eosPos;

    if (curGoldPtr.boundary == curTopBnd) {
      // case when boundaries match
      // in this case we find a node from top1-path
      // that has maximum mismatch with the gold data
      auto stepData = ComparisonStep::both();
      if (!findWorstTopNode(curGoldPtr.position, &stepData)) {
        return Status::InvalidState()
               << "could not find a worst top node for position #" << curTopBnd;
      }
      auto top1Score = stepData.topPtr->totalScore;
      goldScore = goldScores[curGold];
      const auto bndBeamData =
          lattice->boundary(curTopBnd)->starts()->beamData();
      auto goldBeam = bndBeamData.row(curGoldPtr.position);

      stepData.lastGoldScore = goldScore;
      stepData.violation = top1Score - goldScore;
      stepData.goldInBeam = isGoldStillInBeam(goldBeam, curGold);
      stepData.numGold = curGold;
      comparison.push_back(stepData);

      // move to next boundary
      hasNextTop = top1.nextBoundary();
      curGold += 1;
      continue;
    }

    // cases when top1 has different segmentation with gold
    if (curGoldPtr.boundary > curTopBnd) {
      // top1 boundary comes first
      auto ptr = top1.nextBeamPtr();
      if (ptr == nullptr) {
        return Status::InvalidState()
               << "boundary #" << curTopBnd << " did not have any nodes";
      }

      auto stepData = ComparisonStep::topOnly();
      stepData.lastGoldScore = goldScore;
      stepData.boundary = curTopBnd;
      stepData.topPtr = ptr;
      stepData.violation = ptr->totalScore - goldScore;
      comparison.push_back(stepData);

      hasNextTop = top1.nextBoundary();
    } else {
      // gold boundary comes first
      auto goldPtr = goldPtrs.at(curGold);
      const auto bnd = lattice->boundary(goldPtr.boundary)->starts();
      auto goldBeam = bnd->beamData().row(goldPtr.position);
      goldScore = goldScores[curGold];

      auto stepData = ComparisonStep::goldOnly();
      stepData.goldPosition = goldPtr.position;
      stepData.boundary = goldPtr.boundary;
      stepData.lastGoldScore = goldScore;
      stepData.goldInBeam = isGoldStillInBeam(goldBeam, curGold);
      stepData.numGold = curGold;
      comparison.push_back(stepData);

      curGold += 1;
    }
  }

  auto eosBnd = lattice->boundary(eosPos);
  const auto eosBeam = eosBnd->starts()->beamData();
  JPP_DCHECK_EQ(eosBeam.numRows(), 1);
  auto eosTopNode = eosBeam.data()[0];
  auto stepData = ComparisonStep::both();
  JPP_DCHECK_EQ(curGold, goldScores.size() - 1);
  goldScore = goldScores[curGold];
  stepData.boundary = eosPos;
  stepData.topPtr = &eosBeam.at(0);
  stepData.goldPosition = 0;
  stepData.lastGoldScore = goldScore;
  stepData.violation = eosTopNode.totalScore - goldScore;
  stepData.goldInBeam = isGoldStillInBeam(eosBeam.row(0), curGold);
  stepData.numGold = curGold;
  stepData.numMismatches = 0;
  stepData.mismatchWeight = 0;
  comparison.push_back(stepData);

  return Status::Ok();
}

bool LossCalculator::isGoldStillInBeam(
    util::ArraySlice<analysis::ConnectionBeamElement> beam, i32 goldPos) {
  if (goldPos == 0) {
    return true;
  }

  auto prevGoldPtr = gold.nodes().at(goldPos - 1);
  for (auto& be : beam) {
    if (analysis::EntryBeam::isFake(be)) {
      break;
    }
    auto prev = be.ptr.previous;
    if (prev == nullptr) {
      return false;
    }
    if (prev->boundary == prevGoldPtr.boundary &&
        prev->right == prevGoldPtr.position) {
      return true;
    }
  }
  return false;
}

void LossCalculator::computeFeatureDiff(u32 mask) {
  int topPos = 0;
  int goldPos = 0;
  scored.clear();
  scored.push_back({0, 0});
  auto topCnt = top1Features.size();
  auto goldCnt = goldFeatures.size();

  for (auto& f : top1Features) {
    f = f & mask;
  }

  util::sort(top1Features);

  for (auto& f : goldFeatures) {
    f = f & mask;
  }

  util::sort(goldFeatures);

  while (topPos < topCnt && goldPos < goldCnt) {
    u32 goldEl = goldFeatures[goldPos];
    u32 topEl = top1Features[topPos];
    u32 target;
    float score;

    if (goldEl == topEl) {
      goldPos += 1;
      topPos += 1;
      continue;
    } else if (goldEl < topEl) {
      target = goldEl;
      score = 1.0f;
      goldPos += 1;
    } else {
      target = topEl;
      score = -1.0f;
      topPos += 1;
    }

    mergeOne(target, score);
  }

  for (; topPos < topCnt; ++topPos) {
    mergeOne(top1Features[topPos], 1.0f);
  }

  for (; goldPos < goldCnt; ++goldPos) {
    mergeOne(goldFeatures[goldPos], -1.0f);
  }

  float numGold = 0;
  float numTop = 0;

  for (auto& s : scored) {
    if (s.score > 0) {
      numGold += s.score;
    } else {
      numTop -= s.score;
    }
  }

  float weight = 1.0f;
  bool updateTop = true;
  if (numTop != 0) {
    weight = numGold / numTop;
  }
  if (weight > 2 && numGold != 0) {
    weight = numTop / numGold;
    updateTop = false;
  }

  if (updateTop) {
    for (auto& s : scored) {
      if (s.score < 0) {
        s.score *= weight;
      }
    }
  } else {
    for (auto& s : scored) {
      if (s.score > 0) {
        s.score *= weight;
      }
    }
  }
}

void LossCalculator::mergeOne(u32 target, float score) {
  if (scored.back().feature != target) {
    scored.push_back({target, 0});
  }
  scored.back().score += score;
}

void LossCalculator::addTopNgrams(i32 cmpIdx) {
  auto lat = analyzer->lattice();
  NgramFeaturesComputer nfc{lat, analyzer->core().features()};
  auto& cmp = comparison[cmpIdx];
  auto t0beamPtr = cmp.topPtr;
  auto& t0 = t0beamPtr->ptr;
  auto& t1 = *t0.previous;
  auto& t2 = *t1.previous;
  NgramFeatureRef nfr{t2.latticeNodePtr(), t1.latticeNodePtr(),
                      t0.latticeNodePtr()};
  nfc.calculateNgramFeatures(nfr, &featureBuffer);
  util::copy_insert(featureBuffer, top1Features);

// and logic for handling ngrams for correct nodes
#if defined(JPP_TRAIN_MID_NGRAMS)

  auto nextIdx = cmpIdx + 1;
  if (nextIdx >= comparison.size()) {
    return;
  }

  auto& cmpNext = comparison[nextIdx];
  if (cmpNext.hasError()) {
    return;
  }

  auto prevIdx = cmpIdx - 1;
  if (prevIdx < 0) {
    return;
  }

  NgramFeaturesComputer nefc{analyzer->lattice(), analyzer->core().features()};
  auto& tn1 = cmpNext.topPtr->ptr;
  auto nfr2 = nfr.next(tn1.latticeNodePtr());
  nfc.calculateNgramFeatures(nfr2, &featureBuffer);

  auto& cmpPrev = comparison[prevIdx];
  if (!cmpPrev.hasError()) {
    auto bitri = nefc.subset(featureBuffer, features::NgramSubset::BiTri);
    util::copy_insert(bitri, top1Features);
  } else {
    auto bi = nefc.subset(featureBuffer, features::NgramSubset::Bigrams);
    util::copy_insert(bi, top1Features);
  }

  auto next2Idx = cmpIdx + 2;
  if (next2Idx >= comparison.size()) {
    return;
  }

  auto& cmpNext2 = comparison[next2Idx];
  if (cmpNext2.hasError()) {
    return;
  }

  auto& tn2 = cmpNext2.topPtr->ptr;
  auto nfr3 = nfr2.next(tn2.latticeNodePtr());
  nfc.calculateNgramFeatures(nfr3, &featureBuffer);
  auto tri = nefc.subset(featureBuffer, features::NgramSubset::Trigrams);
  util::copy_insert(tri, top1Features);
#endif  // JPP_TRAIN_MID_NGRAMS
}

float LossCalculator::computeLoss(i32 till) {
  JPP_DCHECK_IN(till, 0, comparison.size() + 1);
  goldFeatures.clear();
  top1Features.clear();
  float loss = 0;
  auto size = fullSize();
  for (int i = 0; i < size; ++i) {
    auto& cmp = comparison[i];
    if (cmp.cmpClass == ComparitionClass::GoldOnly) {
      loss += fullWeight;
      if (i < till) {
        addGoldNgrams(i, cmp.numGold);
      }
    } else if (cmp.cmpClass == ComparitionClass::TopOnly) {
      loss += fullWeight;
      if (i < till) {
        addTopNgrams(i);
      }
    } else if (cmp.cmpClass == ComparitionClass::Both) {
      if (cmp.hasError()) {
        loss += cmp.mismatchWeight;
        if (i < till) {
          addGoldNgrams(i, cmp.numGold);
          addTopNgrams(i);
        }
      }
    }
  }

  auto lossValue = loss / (size * fullWeight);
#if 0
  if (lossValue > 0) {
    LOG_TRACE() << "LOSS: " << lossValue << compDump();
  }
#endif
  return lossValue;
}

void LossCalculator::addGoldNgrams(i32 cmpIdx, i32 position) {
  auto numNgrams = analyzer->core().spec().features.ngram.size();
  util::ConstSliceable<u32> slice{rawGoldFeatures, numNgrams,
                                  gold.nodes().size() + 1};
  util::copy_insert(slice.row(position), goldFeatures);

// and logic for handling ngrams for correct nodes
#if defined(JPP_TRAIN_MID_NGRAMS)

  auto nextIdx = cmpIdx + 1;
  if (nextIdx >= comparison.size()) {
    return;
  }

  auto& cmpNext = comparison[nextIdx];
  if (cmpNext.hasError()) {
    return;
  }

  auto prevIdx = cmpIdx - 1;
  if (prevIdx < 0) {
    return;
  }

  NgramFeaturesComputer nefc{analyzer->lattice(), analyzer->core().features()};
  auto& cmpPrev = comparison[prevIdx];
  if (!cmpPrev.hasError()) {
    auto bitri =
        nefc.subset(slice.row(cmpNext.numGold), features::NgramSubset::BiTri);
    util::copy_insert(bitri, goldFeatures);
  } else {
    auto bi =
        nefc.subset(slice.row(cmpNext.numGold), features::NgramSubset::Bigrams);
    util::copy_insert(bi, goldFeatures);
  }

  auto next2Idx = cmpIdx + 2;
  if (next2Idx >= comparison.size()) {
    return;
  }

  auto& cmpNext2 = comparison[next2Idx];
  if (cmpNext2.hasError()) {
    return;
  }

  auto tri =
      nefc.subset(slice.row(cmpNext2.numGold), features::NgramSubset::Trigrams);
  util::copy_insert(tri, goldFeatures);
#endif  // JPP_TRAIN_MID_NGRAMS
}

Status LossCalculator::resolveGold() {
  NgramFeaturesComputer nefc{analyzer->lattice(), analyzer->core().features()};
  auto goldNodes = gold.nodes();
  auto numNodes = goldNodes.size();
  auto withEos = numNodes + 1;
  auto numNgrams = analyzer->core().spec().features.ngram.size();
  rawGoldFeatures.resize(withEos * numNgrams);
  if (numNodes == 0) {
    return Status::Ok();
  }
  util::Sliceable<u32> goldNgrams{&rawGoldFeatures, numNgrams, withEos};
  auto nfr = NgramFeatureRef::init(goldNodes.at(0));
  nefc.calculateNgramFeatures(nfr, goldNgrams.row(0));
  for (int idx = 1; idx < numNodes; ++idx) {
    nfr = nfr.next(goldNodes.at(idx));
    nefc.calculateNgramFeatures(nfr, goldNgrams.row(idx));
  }

  auto totalBndCount = analyzer->lattice()->createdBoundaryCount();
  u16 eosBoundary = static_cast<u16>(totalBndCount - 1);
  nfr = nfr.next({eosBoundary, 0});  // EOS
  nefc.calculateNgramFeatures(nfr, goldNgrams.row(numNodes));
  return Status::Ok();
}

void LossCalculator::computeGoldScores(const analysis::ScorerDef* scores) {
  auto numGoldNodes = gold.nodes().size() + 1;  // EOS as well
  goldNodeScores.resize(numGoldNodes);
  goldScores.resize(numGoldNodes);
  auto numNgrams = analyzer->core().spec().features.ngram.size();
  util::Sliceable<u32> features{&rawGoldFeatures, numNgrams, numGoldNodes};
  scores->feature->compute(&goldNodeScores, features);
  std::partial_sum(goldNodeScores.begin(), goldNodeScores.end(),
                   goldScores.begin());
}

std::string LossCalculator::compDump() const {
  std::stringstream ss;
  for (auto& c : comparison) {
    ss << "\n";
    switch (c.cmpClass) {
      case ComparitionClass::Both:
        ss << "B: ";
        break;
      case ComparitionClass ::GoldOnly:
        ss << "G: ";
        break;
      case ComparitionClass ::TopOnly:
        ss << "T: ";
        break;
    }
    ss << c.boundary << " " << c.numMismatches;
    if (c.numMismatches != 0) {
      ss << ":" << c.mismatchWeight;
    }
    ss << " " << c.numGold << "*" << c.goldPosition;
    if (c.topPtr) {
      ss << " " << *c.topPtr;
    } else {
      ss << " null";
    }

    ss << "@" << c.violation << " " << c.goldInBeam;
  }
  return ss.str();
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
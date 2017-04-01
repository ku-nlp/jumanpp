//
// Created by Arseny Tolmachev on 2017/03/23.
//

#include "loss.h"

#include "core/analysis/unk_nodes_creator.h"
#include "core/impl/feature_impl_types.h"

namespace jumanpp {
namespace core {
namespace training {
void NgramExampleFeatureCalculator::calculateNgramFeatures(
    const NgramFeatureRef& ptrs, util::MutableArraySlice<u32> result) {
  util::Sliceable<u32> resSlice{result, result.size(), 1};
  auto t2f = lattice->boundary(ptrs.t2.boundary)
                 ->starts()
                 ->patternFeatureData()
                 .row(ptrs.t2.position);
  auto t1f = lattice->boundary(ptrs.t1.boundary)
                 ->starts()
                 ->patternFeatureData()
                 .row(ptrs.t1.position);
  auto t0f = lattice->boundary(ptrs.t0.boundary)
                 ->starts()
                 ->patternFeatureData()
                 .rows(ptrs.t0.position, ptrs.t0.position + 1);

  features::impl::NgramFeatureData nfd{resSlice, t2f, t1f, t0f};
  features.ngram->applyBatch(&nfd);
}

bool LossCalculator::findWorstTopNode(i32 goldPos, ComparisonStep* step) {
  analysis::ConnectionPtr conPtr;
  auto lattice = analyzer->lattice();
  auto bndIdx = top1.currentBoundary();
  auto boundary = lattice->boundary(bndIdx);
  auto entries = boundary->starts()->entryData();
  auto goldData = entries.row(goldPos);
  auto& specFlds = spec->fields;

  i32 maxNumMismatches = -1;
  float maxMismatchWeight = -1;
  i32 worstPosition = -1;

  while (top1.nextNode(&conPtr)) {
    auto topData = entries.row(conPtr.right);

    i32 numMismatches = 0;
    float mismatchWeight = 0;

    for (auto& sf : specFlds) {
      if (topData[sf.index] != goldData[sf.index]) {
        numMismatches += 1;
        mismatchWeight += sf.weight;
      }
    }

    if (maxMismatchWeight < mismatchWeight) {
      maxMismatchWeight = mismatchWeight;
      maxNumMismatches = numMismatches;
      worstPosition = conPtr.right;
    }
  }
  if (worstPosition == -1) {
    return false;
  }

  step->boundary = bndIdx;
  step->topPosition = worstPosition;
  step->goldPosition = goldPos;
  step->numMismatches = maxNumMismatches;
  step->mismatchWeight = maxMismatchWeight;
  return true;
}

Status LossCalculator::computeComparison() {
  comparison.clear();
  comparison.reserve(top1.numBoundaries() + gold.nodes().size());
  float goldScore = 0;
  bool hasNextTop = top1.nextBoundary();
  int curGold = 0;
  auto goldPtrs = gold.nodes();
  auto totalGold = goldPtrs.size();
  auto lattice = analyzer->lattice();
  while (hasNextTop && curGold < totalGold) {
    auto curTopBnd = top1.currentBoundary();
    auto curGoldPtr = goldPtrs[curGold];

    if (curGoldPtr.boundary == curTopBnd) {
      // case when boundaries match
      // in this case we find a node from top1-path
      // that has maximum mismatch with the gold data
      auto stepData = ComparisonStep::both();
      if (!findWorstTopNode(curGoldPtr.position, &stepData)) {
        return Status::InvalidState()
               << "could not find a worst top node for position #" << curTopBnd;
      }
      auto bndBeamData = lattice->boundary(curTopBnd)->starts()->beamData();
      auto goldBeam = bndBeamData.row(curGoldPtr.position);
      goldScore = goldBeam[0].totalScore;
      auto top1Beam = bndBeamData.row(stepData.topPosition);
      auto top1Score = top1Beam[0].totalScore;
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
      analysis::ConnectionPtr ptr;
      if (!top1.nextNode(&ptr)) {
        return Status::InvalidState()
               << "boundary #" << curTopBnd << " did not have any nodes";
      }

      auto bnd = lattice->boundary(ptr.boundary);
      auto topBeam = bnd->starts()->beamData().row(ptr.right);

      auto stepData = ComparisonStep::topOnly();
      stepData.lastGoldScore = goldScore;
      stepData.boundary = curTopBnd;
      stepData.topPosition = ptr.right;
      stepData.violation = goldScore - topBeam[0].totalScore;
      comparison.push_back(stepData);

      hasNextTop = top1.nextBoundary();
    } else {
      // gold boundary comes first
      auto goldPtr = goldPtrs.at(curGold);
      auto bnd = lattice->boundary(goldPtr.boundary);
      auto goldBeam = bnd->starts()->beamData().row(goldPtr.position);
      goldScore = goldBeam[0].totalScore;

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

  return Status::Ok();
}

bool LossCalculator::isGoldStillInBeam(
    util::ArraySlice<analysis::ConnectionBeamElement> beam, i32 goldPos) {
  if (goldPos == 0) {
    return true;
  }

  auto prevGoldPtr = gold.nodes().at(goldPos - 1);
  for (auto& be : beam) {
    auto prev = be.ptr.previous;
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

  while (topPos < topCnt || goldPos < goldCnt) {
    u32 goldEl = goldFeatures[goldPos];
    u32 topEl = top1Features[topPos];
    if (goldEl == topEl) {
      goldPos += 1;
      topPos += 1;
      continue;
    }

    u32 target;
    float score;

    if (goldEl < topEl) {
      target = goldEl;
      score = -1;
      goldPos += 1;
    } else {
      target = topEl;
      score = 1;
      topPos += 1;
    }

    mergeOne(target, score);
  }

  for (; topPos < topCnt; ++topPos) {
    mergeOne(top1Features[topPos], 1);
  }

  for (; goldPos < goldCnt; ++goldPos) {
    mergeOne(goldFeatures[goldPos], -1);
  }
}

void LossCalculator::mergeOne(u32 target, float score) {
  if (scored.back().feature != target) {
    scored.push_back({target, 0});
  }
  scored.back().score += score;
}

void LossCalculator::computeNgrams(std::vector<u32>* result, i32 boundary,
                                   i32 position) {
  auto lat = analyzer->lattice();
  NgramExampleFeatureCalculator nfc{lat, analyzer->core().features()};
  auto beam = lat->boundary(boundary)->starts()->beamData().row(position);
  auto rawPtr0 = &beam[0].ptr;
  auto rawPtr1 = rawPtr0->previous;
  auto rawPtr2 = rawPtr1->previous;
  NgramFeatureRef nfr{{rawPtr2->boundary, rawPtr2->right},
                      {rawPtr1->boundary, rawPtr1->right},
                      {rawPtr0->boundary, rawPtr0->right}};
  nfc.calculateNgramFeatures(nfr, &featureBuffer);
  util::copy_insert(featureBuffer, *result);
}

float LossCalculator::computeLoss(i32 till) {
  JPP_DCHECK_IN(till, 0, comparison.size());
  goldFeatures.clear();
  top1Features.clear();
  float loss = 0;
  auto size = fullSize();
  for (int i = 0; i < size; ++i) {
    auto& cmp = comparison[i];
    if (cmp.cmpClass == ComparitionClass::GoldOnly) {
      loss += fullWeight;
      if (i < till) {
        computeGoldNgrams(cmp.numGold);
      }
    } else if (cmp.cmpClass == ComparitionClass::TopOnly) {
      loss += fullWeight;
      if (i < till) {
        computeNgrams(&top1Features, cmp.boundary, cmp.topPosition);
      }
    } else if (cmp.cmpClass == ComparitionClass::Both) {
      if (cmp.numMismatches > 0) {
        loss += cmp.mismatchWeight;
        if (i < till) {
          computeGoldNgrams(cmp.numGold);
          computeNgrams(&top1Features, cmp.boundary, cmp.topPosition);
        }
      }
    }
  }
  return loss / (size * fullWeight);
}

void LossCalculator::computeGoldNgrams(i32 numGold) {
  NgramFeatureRef nfr;
  auto nodes = gold.nodes();
  switch (numGold) {
    case 0: {
      auto goldPtr = nodes.at(0);
      JPP_DCHECK_EQ(2, goldPtr.boundary);
      nfr = NgramFeatureRef{{0, 0}, {1, 0}, {2, goldPtr.position}};
      break;
    }
    case 1: {
      auto gp0 = nodes.at(0);
      auto gp1 = nodes.at(1);
      JPP_DCHECK_EQ(0, gp0.boundary);
      nfr = NgramFeatureRef{
          {1, 0}, {2, gp0.position}, {gp1.boundary, gp1.position}};
      break;
    }
    default: {
      auto gp2 = nodes.at(numGold);
      auto gp1 = nodes.at(numGold - 1);
      auto gp0 = nodes.at(numGold - 2);
      nfr = NgramFeatureRef{{gp0.boundary, gp0.position},
                            {gp1.boundary, gp1.position},
                            {gp2.boundary, gp2.position}};
    }
  }
  NgramExampleFeatureCalculator nfc{analyzer->lattice(),
                                    analyzer->core().features()};
  nfc.calculateNgramFeatures(nfr, &featureBuffer);
  util::copy_insert(featureBuffer, goldFeatures);
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
//
// Created by Arseny Tolmachev on 2017/12/11.
//

#include "partial_example.h"
#include "core/analysis/lattice_types.h"

namespace jumanpp {
namespace core {
namespace input {

bool PartialExample::doesNodeMatch(const analysis::Lattice* lr, i32 boundary,
                                   i32 position) const {
  auto bndStart = lr->boundary(boundary)->starts();
  return doesNodeMatch(bndStart, boundary, position);
}

bool PartialExample::doesNodeMatch(const analysis::LatticeRightBoundary* lr,
                                   i32 boundary, i32 position) const {
  return checkViolation(lr, boundary, position).isNone();
}

PartialViolation PartialExample::checkViolation(
    const analysis::LatticeRightBoundary* lr, i32 boundary,
    i32 position) const {
  auto len = lr->nodeInfo().at(position).numCodepoints();
  auto end = boundary + len;

  for (auto bnd : noBreak_) {
    // violation on non-breaking
    if (bnd == boundary) {
      return {ViolationKind::WordStart, bnd};
    } else if (bnd == end) {
      return {ViolationKind::WordEnd, bnd};
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
    return {ViolationKind::NoBoundary, bnd};
  }

  auto nodeIter = std::find_if(
      nodes_.begin(), nodes_.end(),
      [boundary](const NodeConstraint& n) { return n.boundary == boundary; });

  if (nodeIter == nodes_.end()) {
    return {ViolationKind::None, 0};
  }

  auto& nodeCstrs = *nodeIter;
  if (len != nodeCstrs.length) {
    // should be handled by NoBreak, but nevertheless
    return {ViolationKind::WordEnd, end};
  }

  auto data = lr->entryData().row(position);
  for (auto& tag : nodeCstrs.tags) {
    if (data.at(tag.field) != tag.value) {
      return {ViolationKind::Tag, tag.field};
    }
  }

  return {ViolationKind::None, 0};
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

}  // namespace input
}  // namespace core
}  // namespace jumanpp

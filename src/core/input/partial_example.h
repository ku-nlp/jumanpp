//
// Created by Arseny Tolmachev on 2017/12/11.
//

#ifndef JUMANPP_PARTIAL_EXAMPLE_H
#define JUMANPP_PARTIAL_EXAMPLE_H

#include "core/analysis/lattice_config.h"
#include "core/input/training_io.h"
#include "util/array_slice.h"
#include "util/string_piece.h"

namespace jumanpp {
namespace core {
namespace analysis {
class LatticeRightBoundary;
}
namespace input {

struct TagConstraint {
  i32 field;
  i32 value;
};

struct NodeConstraint {
  i32 boundary;
  i32 length;
  std::string surface;
  std::vector<TagConstraint> tags;
};

enum struct ViolationKind { None, WordStart, WordEnd, NoBoundary, Tag };

struct PartialViolation {
  ViolationKind kind;
  i32 parameter;

  bool isNone() const { return kind == ViolationKind::None; }
};

class PartialExample {
  std::string comment_;
  StringPiece file_;
  i64 line_;
  std::string surface_;
  std::vector<i32> boundaries_;
  std::vector<i32> noBreak_;
  std::vector<NodeConstraint> nodes_;

 public:
  i64 line() const { return line_; }

  StringPiece surface() const { return surface_; }

  util::ArraySlice<i32> boundaries() const { return boundaries_; }

  util::ArraySlice<i32> forbidBreak() const { return noBreak_; }

  util::ArraySlice<NodeConstraint> nodes() const { return nodes_; }

  ExampleInfo exampleInfo() const {
    return ExampleInfo{file_, comment_, line_};
  }

  bool doesNodeMatch(const analysis::Lattice* l, i32 boundary,
                     i32 position) const;
  bool doesNodeMatch(const analysis::LatticeRightBoundary* lr, i32 boundary,
                     i32 position) const;

  PartialViolation checkViolation(const analysis::LatticeRightBoundary* rb,
                                  i32 bnd, i32 pos) const;

  bool validBoundary(i32 bndIdx) const;

  friend class PartialExampleReader;
};

}  // namespace input
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_PARTIAL_EXAMPLE_H

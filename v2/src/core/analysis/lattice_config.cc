//
// Created by Arseny Tolmachev on 2017/10/27.
//

#include "lattice_config.h"
#include <ostream>

namespace jumanpp {
namespace core {
namespace analysis {

std::ostream& operator<<(std::ostream& o, const ConnectionPtr& cbe) {
  o << "[";

  if (cbe.previous != nullptr) {
    auto& p = *cbe.previous;
    o << p.boundary << "," << p.right << "," << p.left << "," << p.beam;
    o << " <- ";
  }

  o << cbe.boundary << "," << cbe.right << "," << cbe.left << "," << cbe.beam;
  o << "]";
  return o;
}

std::ostream& operator<<(std::ostream& o, const ConnectionBeamElement& cbe) {
  o << "{" << cbe.ptr << "," << cbe.totalScore << "}";
  return o;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
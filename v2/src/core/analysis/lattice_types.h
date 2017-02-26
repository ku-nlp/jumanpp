//
// Created by Arseny Tolmachev on 2017/02/26.
//

#ifndef JUMANPP_LATTICE_TYPES_H
#define JUMANPP_LATTICE_TYPES_H

#include "util/soa.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace analysis {

struct ConnectionPtr {
  u16 boundary;
  u16 left;
  u16 right;
  u16 beam;
};

}  // analysis
}  // core
}  // jumanpp

#endif  // JUMANPP_LATTICE_TYPES_H

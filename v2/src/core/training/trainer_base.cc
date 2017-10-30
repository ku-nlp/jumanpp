//
// Created by Arseny Tolmachev on 2017/10/30.
//

#include "trainer_base.h"

namespace jumanpp {
namespace core {
namespace training {
std::ostream& operator<<(std::ostream& o, const GlobalBeamTrainConfig& cfg) {
  if (!cfg.isEnabled()) {
    o << "DISABLED";
  } else {
    o << "l:" << cfg.leftBeam;
    if (cfg.rightBeam > 0) {
      o << ", r:" << cfg.rightCheck << ":" << cfg.rightBeam;
    }
  }
  return o;
}

}  // namespace training
}  // namespace core
}  // namespace jumanpp
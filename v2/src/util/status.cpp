//
// Created by Arseny Tolmachev on 2017/02/18.
//


#include <iosfwd>
#include "status.hpp"

namespace jumanpp {

constexpr const char *const StatusNames[] = {"Ok", "InvalidParameter", "InvalidState",
                                       "MaxStatus"};

std::ostream &operator<<(std::ostream &str, const Status &st) {
  int idx = (int)st.code;
  auto *name = StatusNames[idx];
  str << idx << ":" << name;
  if (!st.message.empty()) {
    str << " " << st.message;
  }
  return str;
}


}

//
// Created by Arseny Tolmachev on 2017/07/05.
//

#ifndef JUMANPP_JUMANDIC_ID_RESOLVER_H
#define JUMANPP_JUMANDIC_ID_RESOLVER_H

#include "core/dictionary.h"
#include "util/flatmap.h"

namespace jumanpp {
namespace jumandic {

struct CompositeId {
  i32 id1;
  i32 id2;
};

inline bool operator==(const CompositeId& o1, const CompositeId& o2) noexcept {
  return o1.id1 == o2.id1 && o1.id2 == o2.id2;
}

struct CompositeIdHash {
  u64 operator()(const CompositeId& o) const noexcept {
    return (o.id1 * 0x412fad52123ULL) + (o.id2 * 0x12512155ffaULL);
  }
};

struct JumandicPosId {
  i32 pos;
  i32 subpos;
  i32 conjType;
  i32 conjForm;
};

class JumandicIdResolver {
  util::FlatMap<CompositeId, CompositeId, CompositeIdHash> posMap;
  util::FlatMap<CompositeId, CompositeId, CompositeIdHash> conjMap;

 public:
  Status initialize(const core::dic::DictionaryHolder& dic);
  JumandicPosId dicToJuman(JumandicPosId ids) const;
};

}  // namespace jumandic
}  // namespace jumanpp

#endif  // JUMANPP_JUMANDIC_ID_RESOLVER_H

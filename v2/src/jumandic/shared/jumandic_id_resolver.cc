//
// Created by Arseny Tolmachev on 2017/07/05.
//

#include "jumandic_id_resolver.h"
#include "jumandic_ids.h"

namespace jumanpp {
namespace jumandic {

util::FlatMap<StringPiece, i32> readFieldToMap(
    const core::dic::DictionaryField* fld) {
  core::dic::impl::StringStorageTraversal trav{fld->strings};
  StringPiece sp;
  util::FlatMap<StringPiece, i32> result;
  while (trav.next(&sp)) {
    result[sp] = trav.position();
  }
  return result;
}

template <typename K, typename V, typename H, typename E>
V findOr(const util::FlatMap<K, V, H, E>& map, const K& key, V defVal) {
  auto it = map.find(key);
  if (it == map.end()) {
    return defVal;
  }
  return it->second;
}

Status JumandicIdResolver::initialize(const core::dic::DictionaryHolder& dic) {
  auto posFld = dic.fieldByName("pos");
  if (posFld == nullptr) {
    return Status::InvalidState() << "JumandicIdResolver: pos field was absent";
  }

  auto subPosFld = dic.fieldByName("subpos");
  if (subPosFld == nullptr) {
    return Status::InvalidState()
           << "JumandicIdResolver: subpos field was absent";
  }

  auto conjFormFld = dic.fieldByName("conjform");
  if (conjFormFld == nullptr) {
    return Status::InvalidState()
           << "JumandicIdResolver: conjform field was absent";
  }

  auto conjTypeFld = dic.fieldByName("conjtype");
  if (conjTypeFld == nullptr) {
    return Status::InvalidState()
           << "JumandicIdResolver: conjtype field was absent";
  }

  auto pos2id = readFieldToMap(posFld);
  auto subPos2id = readFieldToMap(subPosFld);
  auto conjForm2id = readFieldToMap(conjFormFld);
  auto conjType2id = readFieldToMap(conjTypeFld);

  for (int i = 0; i < posInfoCount; ++i) {
    auto& pos = posInfo[i];
    auto posId = findOr(pos2id, pos.part1, 0);
    auto subposId = findOr(subPos2id, pos.part2, 0);
    if (posId != 0 || subposId != 0) {
      CompositeId posObj{posId, subposId};
      posMap[posObj] = CompositeId{pos.id1, pos.id2};
    }
  }

  for (int j = 0; j < conjInfoCount; ++j) {
    auto& conj = conjInfo[j];
    auto ctId = findOr(conjType2id, conj.part1, 0);
    auto cfId = findOr(conjForm2id, conj.part2, 0);
    if (ctId != 0) {
      conjMap[CompositeId{ctId, cfId}] = CompositeId{conj.id1, conj.id2};
      conjMap[CompositeId{ctId, 0}] = CompositeId{conj.id1, 0};
    }
  }

  return Status::Ok();
}

JumandicPosId JumandicIdResolver::dicToJuman(JumandicPosId ids) const {
  auto p1 = findOr(posMap, CompositeId{ids.pos, ids.subpos}, CompositeId{0, 0});
  auto p2 = findOr(conjMap, CompositeId{ids.conjType, ids.conjForm},
                   CompositeId{0, 0});
  return JumandicPosId{p1.id1, p1.id2, p2.id1, p2.id2};
}
}  // namespace jumandic
}  // namespace jumanpp
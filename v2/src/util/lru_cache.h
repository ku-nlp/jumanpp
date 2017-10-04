//
// Created by Arseny Tolmachev on 2017/10/03.
//

#ifndef JUMANPP_LRU_CACHE_H
#define JUMANPP_LRU_CACHE_H

#include <util/flatmap.h>

namespace jumanpp {
namespace util {

// Implements DoubleBarrelLRUCache algorithm
template <typename K, typename V, typename Hash = std::hash<K>,
          typename Eq = std::equal_to<K>>
class LruCache {
  util::FlatMap<K, V, Hash, Eq> map1_;
  util::FlatMap<K, V, Hash, Eq> map2_;
  size_t capacity_;

  void maybe_swap() {
    if (map1_.size() >= capacity_) {
      map1_.swap(map2_);
      map1_.clear_no_resize();
    }
  }

 public:
  explicit LruCache(size_t capacity) : capacity_{capacity} {
    map1_.reserve(capacity * 3 / 2);
    map2_.reserve(capacity * 3 / 2);
  }

  bool tryFind(const K& key, V* result) {
    auto it1 = map1_.find(key);
    if (it1 != map1_.end()) {
      *result = it1->second;
      return true;
    }

    auto it2 = map2_.find(key);
    if (it2 != map2_.end()) {
      *result = it2->second;
      map1_.insert(std::make_pair(key, it2->second));
      maybe_swap();
      return true;
    }
    return false;
  }

  void insert(const K& key, const V& value) {
    map1_.insert(std::make_pair(key, value));
    maybe_swap();
  }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_LRU_CACHE_H

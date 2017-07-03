//
// Created by Arseny Tolmachev on 2017/07/03.
//

#ifndef JUMANPP_SERIALIZATION_FLATMAP_H
#define JUMANPP_SERIALIZATION_FLATMAP_H

#include "serialization.h"
#include "flatmap.h"

namespace jumanpp {
namespace util {
namespace serialization {


template <typename K, typename V, typename H, typename E>
struct SerializeImpl<FlatMap<K, V, H, E>> {
  static inline void DoSerializeSave(FlatMap<K, V, H, E> &o, Saver *s,
                                     util::CodedBuffer &buf) {
    u64 capacity = o.bucket_count();
    buf.writeVarint(capacity);
    u64 size = o.size();
    buf.writeVarint(size);
    for (auto &child : o) {
      //need this for making type system happy
      //we do not modify values
      K& keyref = const_cast<K&>(child.first);
      SerializeImpl<K>::DoSerializeSave(keyref, s, buf);
      SerializeImpl<V>::DoSerializeSave(child.second, s, buf);
    }
  }

  static inline bool DoSerializeLoad(FlatMap<K, V, H, E> &o, Loader *s,
                                     util::CodedBufferParser &p) {
    u64 capa;
    JPP_RET_CHECK(p.readVarint64(&capa));
    o.reserve(capa);
    u64 size;
    JPP_RET_CHECK(p.readVarint64(&size));
    for (int i = 0; i < size; ++i) {
      K key;
      JPP_RET_CHECK(SerializeImpl<K>::DoSerializeLoad(key, s, p));
      JPP_RET_CHECK(SerializeImpl<V>::DoSerializeLoad(o[key], s, p));
    }
    return true;
  }
};

}  // namespace serialization
}  // namespace util
}  // namespace jumanpp


#endif //JUMANPP_SERIALIZATION_FLATMAP_H

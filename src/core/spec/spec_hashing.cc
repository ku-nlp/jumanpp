//
// Created by Arseny Tolmachev on 2019-07-24.
//

#include "spec_hashing.h"
#include "spec_ser.h"

namespace jumanpp {
namespace core {
namespace spec {

namespace {

struct SpecHashState {
  util::hashing::FastHash1 h{0x4231512d};
  u32 count = 0;

  friend std::ostream& operator<<(std::ostream& o, const SpecHashState& shs) {
    o << '{' << shs.h.result() << ':' << shs.count << '}';
    return o;
  }
};

template <typename T>
struct SpecHashImpl {
  static void hash(SpecHashState& s, T& val) {
    auto cnt = s.count;
    s.count = 0;
    Serialize(s, val);
    s.h = s.h.mix(0x512a5299ff1ULL).mix(s.count);
    s.count = cnt + 1;
  }
};

template <>
struct SpecHashImpl<StringPiece> {
  static void hash(SpecHashState& s, StringPiece& val) {
    auto x = util::hashing::murmurhash3_memory(val.ubegin(), val.uend(),
                                               0xdeadbeefad);
    s.h = s.h.mix(x);
    s.count += 1;
  }
};

template <>
struct SpecHashImpl<std::string> {
  static void hash(SpecHashState& s, std::string& val) {
    auto sp = StringPiece(val);
    SpecHashImpl<StringPiece>::hash(s, sp);
  }
};

template <typename T>
struct SpecHashImpl<std::vector<T>> {
  static void hash(SpecHashState& s, std::vector<T>& v) {
    s.h = s.h.mix(0xdeadbeef15);
    auto cnt = s.count;
    for (auto& e : v) {
      SpecHashImpl<T>::hash(s, e);
    }
    s.count = cnt + 1;
    s.h = s.h.mix(v.size());
  }
};

template <>
struct SpecHashImpl<i32> {
  static void hash(SpecHashState& s, i32& v) {
    s.h = s.h.mix(v);
    s.count += 1;
  }
};

template <>
struct SpecHashImpl<u32> {
  static void hash(SpecHashState& s, u32& v) {
    s.h = s.h.mix(v);
    s.count += 1;
  }
};

template <>
struct SpecHashImpl<bool> {
  static void hash(SpecHashState& s, bool& v) {
    u32 value = v ? 1 : 0;
    s.h = s.h.mix(value);
    s.count += 1;
  }
};

template <>
struct SpecHashImpl<float> {
  static void hash(SpecHashState& s, float& v) {
    u32 v1;
    std::memcpy(&v1, &v, sizeof(float));
    s.h = s.h.mix(v1);
    s.count += 1;
  }
};

template <typename T>
void operator&(SpecHashState& h, T& o) {
  SpecHashImpl<T>::hash(h, o);
}

}  // namespace

u64 hashSpec(const AnalysisSpec& spec) {
  SpecHashState state = SpecHashState();

  auto mutSpec = const_cast<AnalysisSpec&>(spec);
  state& mutSpec.dictionary;
  state& mutSpec.features;
  state& mutSpec.specVersion_;

  return state.h.result();
}

}  // namespace spec
}  // namespace core
}  // namespace jumanpp
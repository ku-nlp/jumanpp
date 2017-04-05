//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_SERIALIZATION_H
#define JUMANPP_SERIALIZATION_H

#include <memory>
#include <vector>
#include <cstring>
#include "coded_io.h"

namespace jumanpp {
namespace util {
namespace serialization {

class Saver;
class Loader;

template <typename T>
struct SerializeImpl {
  static inline void DoSerializeSave(T &o, Saver *s, util::CodedBuffer &buf) {
    Serialize(*s, o);
  }

  static inline bool DoSerializeLoad(T &o, Loader *s,
                                     util::CodedBufferParser &p);
};

class Saver {
  std::unique_ptr<CodedBuffer> owned_;
  util::CodedBuffer *buf_;

 public:
  Saver() : owned_{new CodedBuffer}, buf_{owned_.get()} {}

  Saver(util::CodedBuffer *ptr) : buf_{ptr} {}

  template <typename T>
  void save(const T &o) {
    SerializeImpl<T>::DoSerializeSave(const_cast<T &>(o), this, *buf_);
  }

  template <typename T>
  void operator&(T &o) {
    SerializeImpl<T>::DoSerializeSave(o, this, *buf_);
  }

  StringPiece result() const { return buf_->contents(); }

  size_t size() const { return buf_->position(); }
};

class Loader {
  util::CodedBufferParser parser_;
  bool status_ = true;

 public:
  Loader() {}
  Loader(StringPiece data) : parser_{data} {}

  template <typename T>
  bool load(T *o) {
    if (parser_.remaining() == 0) {
      status_ = false;
      return false;
    }
    status_ = true;
    auto ret = SerializeImpl<T>::DoSerializeLoad(*o, this, parser_);
    return ret && status_ && parser_.remaining() == 0;
  }

  template <typename T>
  void operator&(T &o) {
    status_ = SerializeImpl<T>::DoSerializeLoad(o, this, parser_);
  }

  void reset(StringPiece data) { parser_.reset(data); }

  bool status() const { return status_; }
};

template <typename T>
inline bool SerializeImpl<T>::DoSerializeLoad(T &o, Loader *s,
                                              util::CodedBufferParser &p) {
  if (!s->status()) {
    return false;
  }
  Serialize(*s, o);
  return s->status();
}

template <>
struct SerializeImpl<i32> {
  static inline void DoSerializeSave(i32 &o, Saver *s, util::CodedBuffer &buf) {
    u32 value = static_cast<u32>(o);
    buf.writeVarint(value);
  }

  static inline bool DoSerializeLoad(i32 &o, Loader *s,
                                     util::CodedBufferParser &p) {
    u32 v;
    auto ret = p.readInt(&v);
    if (ret) {
      o = static_cast<i32>(v);
    }
    return ret;
  }
};

template <>
struct SerializeImpl<std::string> {
  static inline void DoSerializeSave(std::string &o, Saver *s,
                                     util::CodedBuffer &buf) {
    buf.writeString(o);
  }

  static inline bool DoSerializeLoad(std::string &o, Loader *s,
                                     util::CodedBufferParser &p) {
    StringPiece sp;
    auto ret = p.readStringPiece(&sp);
    if (ret) {
      o = sp.str();
    }
    return ret;
  }
};

template <>
struct SerializeImpl<StringPiece> {
  static inline void DoSerializeSave(StringPiece &o, Saver *s,
                                     util::CodedBuffer &buf) {
    buf.writeString(o);
  }

  static inline bool DoSerializeLoad(StringPiece &o, Loader *s,
                                     util::CodedBufferParser &p) {
    StringPiece sp;
    auto ret = p.readStringPiece(&sp);
    if (ret) {
      o = sp;
    }
    return ret;
  }
};

template <>
struct SerializeImpl<bool> {
  static inline void DoSerializeSave(bool &o, Saver *s,
                                     util::CodedBuffer &buf) {
    i32 i = o ? 1 : 0;
    SerializeImpl<i32>::DoSerializeSave(i, s, buf);
  }

  static inline bool DoSerializeLoad(bool &o, Loader *s,
                                     util::CodedBufferParser &p) {
    i32 res = -1;
    JPP_RET_CHECK(SerializeImpl<i32>::DoSerializeLoad(res, s, p));
    if (res == -1) return false;
    o = (res == 1);
    return true;
  }
};

template <>
struct SerializeImpl<u64> {
  static inline void DoSerializeSave(u64 &o, Saver *s, util::CodedBuffer &buf) {
    buf.writeVarint(o);
  }

  static inline bool DoSerializeLoad(u64 &o, Loader *s,
                                     util::CodedBufferParser &p) {
    u64 r;
    JPP_RET_CHECK(p.readVarint64(&r));
    o = r;
    return true;
  }
};

// memcpy here is for C++ strict aliasing rules
// modern compilers optimize it to nothing
template <>
struct SerializeImpl<float> {
  static inline void DoSerializeSave(float &o, Saver *s,
                                     util::CodedBuffer &buf) {
    static_assert(sizeof(float) == sizeof(u32),
                  "size of float and u32 should be equal");
    u32 value;
    std::memcpy(&value, &o, sizeof(float));
    buf.writeFixed32(value);
  }

  static inline bool DoSerializeLoad(float &o, Loader *s,
                                     util::CodedBufferParser &p) {
    static_assert(sizeof(float) == sizeof(u32),
                  "size of float and u32 should be equal");
    u32 bytes;
    JPP_RET_CHECK(p.readFixed32(&bytes));
    float r;
    std::memcpy(&r, &bytes, sizeof(float));
    o = r;
    return true;
  }
};

template <typename T, typename A>
struct SerializeImpl<std::vector<T, A>> {
  static inline void DoSerializeSave(std::vector<T, A> &o, Saver *s,
                                     util::CodedBuffer &buf) {
    u64 size = o.size();
    buf.writeVarint(size);
    for (T &child : o) {
      SerializeImpl<T>::DoSerializeSave(child, s, buf);
    }
  }

  static inline bool DoSerializeLoad(std::vector<T, A> &o, Loader *s,
                                     util::CodedBufferParser &p) {
    u64 sz;
    JPP_RET_CHECK(p.readVarint64(&sz));
    o.reserve(sz);
    for (int i = 0; i < sz; ++i) {
      o.emplace_back();
      JPP_RET_CHECK(SerializeImpl<T>::DoSerializeLoad(o.back(), s, p));
    }
    return true;
  }
};

#define SERIALIZE_ENUM_CLASS(name, type)       \
  template <typename Arch>                     \
  inline void Serialize(Arch &a_, name &nm_) { \
    type var_ = static_cast<type>(nm_);        \
    a_ &var_;                                  \
    nm_ = static_cast<name>(var_);             \
  }

}  // namespace serialization
}  // namespace util
}  // namespace jumanpp
#endif  // JUMANPP_SERIALIZATION_H

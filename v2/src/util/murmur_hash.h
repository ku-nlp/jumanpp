//
// Created by Arseny Tolmachev on 2017/02/20.
//

#ifndef JUMANPP_MURMUR_HASH_H
#define JUMANPP_MURMUR_HASH_H

#include <utility>
#include "common.hpp"
#include "types.hpp"

namespace jumanpp {
namespace util {
namespace hashing {

namespace impl {
typedef std::pair<uint64_t, uint64_t> OWORD;

JPP_ALWAYS_INLINE inline constexpr uint64_t rotl64(uint64_t x, int8_t r) {
  return (x << r) | (x >> (64 - r));
}

JPP_ALWAYS_INLINE inline constexpr uint64_t fmix64(uint64_t k) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdULL;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ULL;
  k ^= k >> 33;

  return k;
}

JPP_ALWAYS_INLINE inline void mur1_oword(OWORD &block, const OWORD &constant) {
  block.first *= constant.first;
  block.first = rotl64(block.first, 31);
  block.first *= constant.second;

  block.second *= constant.second;
  block.second = rotl64(block.second, 33);
  block.second *= constant.first;
  return;
}

JPP_ALWAYS_INLINE inline void mur2_oword(OWORD &block, OWORD *val) {
  val->first ^= block.first;
  val->first = rotl64(val->first, 27);
  val->first += val->second;
  val->first = val->first * 5 + 0x52dce729;

  val->second ^= block.second;
  val->second = rotl64(val->second, 31);
  val->second += val->first;
  val->second = val->second * 5 + 0x38495ab5;
  return;
}

JPP_ALWAYS_INLINE inline void murmur_oword(OWORD &block, const OWORD &constant,
                                    OWORD *val) {
  mur1_oword(block, constant);
  mur2_oword(block, val);
  return;
}

JPP_ALWAYS_INLINE inline void get_tail(const uint8_t *tail, const size_t len,
                                       OWORD *key) {
  switch (len & 0xf) {
    case 15:
      key->second ^= (uint64_t)(tail[14]) << 48;
    case 14:
      key->second ^= (uint64_t)(tail[13]) << 40;
    case 13:
      key->second ^= (uint64_t)(tail[12]) << 32;
    case 12:
      key->second ^= (uint64_t)(tail[11]) << 24;
    case 11:
      key->second ^= (uint64_t)(tail[10]) << 16;
    case 10:
      key->second ^= (uint64_t)(tail[9]) << 8;
    case 9:
      key->second ^= (uint64_t)(tail[8]) << 0;
    case 8:
      key->first ^= (uint64_t)(tail[7]) << 56;
    case 7:
      key->first ^= (uint64_t)(tail[6]) << 48;
    case 6:
      key->first ^= (uint64_t)(tail[5]) << 40;
    case 5:
      key->first ^= (uint64_t)(tail[4]) << 32;
    case 4:
      key->first ^= (uint64_t)(tail[3]) << 24;
    case 3:
      key->first ^= (uint64_t)(tail[2]) << 16;
    case 2:
      key->first ^= (uint64_t)(tail[1]) << 8;
    case 1:
      key->first ^= (uint64_t)(tail[0]) << 0;
  };

  return;
}

constexpr OWORD constant = OWORD(0x87c37b91114253d5LLU, 0x4cf5ad432745937fLLU);

}  // impl

JPP_ALWAYS_INLINE inline uint64_t murmur_combine(uint64_t hash1, uint64_t hash2) {  //{{{
  using namespace impl;
  OWORD key = OWORD(hash1, hash2);
  OWORD value = OWORD(0, 0);
  murmur_oword(key, constant, &value);
  return value.first;
}

JPP_ALWAYS_INLINE inline uint64_t murmurhash3_memory(const u8 *begin, const u8 *end,
                                   const uint64_t seed) {
  using namespace impl;
  OWORD value = OWORD(seed, seed);

  const uint8_t *data = begin;
  size_t len = end - begin;
  const uint64_t *blocks = reinterpret_cast<const uint64_t *>(data);
  const size_t block_size = len / sizeof(OWORD);

  for (unsigned int i = 0; i < block_size; i++) {
    OWORD block = OWORD(blocks[i * 2], blocks[i * 2 + 1]);
    murmur_oword(block, constant, &value);
  }

  OWORD tail = OWORD(0, 0);
  const uint8_t *data_tail = (data + block_size * 16);
  get_tail(data_tail, len, &tail);

  mur1_oword(tail, constant);
  value.first ^= tail.first;
  value.second ^= tail.second;

  value.first ^= len;
  value.second ^= len;

  value.first += value.second;
  value.second += value.first;

  value.first = fmix64(value.first);
  value.second = fmix64(value.second);

  value.first += value.second;
  value.second += value.first;

  return value.first;
}

}  // hashing
}  // util
}  // jumanpp

#endif  // JUMANPP_MURMUR_HASH_H


#pragma once
#include <vector>
#include <string>
#include <stdint.h>

typedef std::pair<uint64_t, uint64_t> OWORD;
const OWORD constant = OWORD(0x87c37b91114253d5LLU, 0x4cf5ad432745937fLLU);

inline uint64_t murmurhash3_u64_vector(const std::vector<uint64_t> &vec,
                                       uint64_t seed = 0);
inline uint64_t murmurhash3_string(const std::string &vec, uint64_t seed = 0);

// unordered_map 用の何もしないhash関数
struct DummyHash {
    std::size_t operator()(const uint64_t &key) const;
};

inline std::size_t DummyHash::operator()(const uint64_t &key) const {
    return key;
}
// 素性関数用の文字列hash関数
struct MurMurHash3_str {
    std::size_t operator()(const std::string &key) const;
};

inline std::size_t MurMurHash3_str::operator()(const std::string &key) const {
    return murmurhash3_string(key);
}

// 素性関数用のint列hash関数
struct MurMurHash3_vector {
    std::size_t operator()(const std::vector<uint64_t> &key) const;
};

inline std::size_t MurMurHash3_vector::
operator()(const std::vector<uint64_t> &key) const {
    return murmurhash3_u64_vector(key);
}

// 素性関数用のhash 関数
static inline uint64_t rotl64(uint64_t x, int8_t r) { //{{{
    return (x << r) | (x >> (64 - r));
} //}}}

static inline uint64_t fmix64(uint64_t k) { //{{{
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;

    return k;
} //}}}

inline void mur1_oword(OWORD &block, const OWORD &constant) { //{{{
    block.first *= constant.first;
    block.first = rotl64(block.first, 31);
    block.first *= constant.second;

    block.second *= constant.second;
    block.second = rotl64(block.second, 33);
    block.second *= constant.first;
    return;
} //}}}

inline void mur2_oword(OWORD &block, OWORD *val) { //{{{
    val->first ^= block.first;
    val->first = rotl64(val->first, 27);
    val->first += val->second;
    val->first = val->first * 5 + 0x52dce729;

    val->second ^= block.second;
    val->second = rotl64(val->second, 31);
    val->second += val->first;
    val->second = val->second * 5 + 0x38495ab5;
    return;
} //}}}

inline void murmur_oword(OWORD &block, const OWORD &constant,
                         OWORD *val) { //{{{
    mur1_oword(block, constant);
    mur2_oword(block, val);
    return;
} //}}}

inline void get_tail(const uint8_t *tail, const size_t len, OWORD *key) { //{{{
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
} //}}}

inline uint64_t murmurhash3_string(const std::string &key,
                                   const uint64_t seed) { //{{{
    OWORD value = OWORD(seed, seed);

    const uint8_t *data = (const uint8_t *)key.c_str();
    size_t len = key.length();
    const uint64_t *blocks = (const uint64_t *)(data);
    const size_t block_size = len / 16; // sizeof(OWORD)

    for (unsigned int i = 0; i < block_size; i++) {
        OWORD block = OWORD(blocks[i * 2], blocks[i * 2 + 1]);
        murmur_oword(block, constant, &value);
    }

    OWORD tail = OWORD(0, 0);
    const uint8_t *data_tail = (const uint8_t *)(data + block_size * 16);
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
} //}}}

inline uint64_t murmurhash3_u64_vector(const std::vector<uint64_t> &vec,
                                       const uint64_t seed) { //{{{
    size_t len = vec.size();
    size_t block_size = len / 2;

    OWORD value = OWORD(seed, seed);

    for (unsigned int i = 0; i < block_size; i++) {
        OWORD block = OWORD(vec[i * 2], vec[i * 2 + 1]);
        murmur_oword(block, constant, &value);
    }

    if (len % 2 == 1) {
        OWORD block = OWORD(vec[block_size * 2], 0);
        mur1_oword(block, constant);
        value.first ^= block.first;
        value.second ^= block.second;
    }

    value.first ^= len;
    value.second ^= len;

    value.first += value.second;
    value.second += value.first;

    value.first = fmix64(value.first);
    value.second = fmix64(value.second);

    value.first += value.second;
    value.second += value.first;

    return value.first;
} //}}}

inline uint64_t murmur_combine(uint64_t hash1, uint64_t hash2) { //{{{
    OWORD key = OWORD(hash1, hash2);
    OWORD value = OWORD(0, 0);
    murmur_oword(key, constant, &value);
    return value.first;
} //}}}

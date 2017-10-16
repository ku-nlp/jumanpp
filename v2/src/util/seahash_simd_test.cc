//
// Created by Arseny Tolmachev on 2017/10/16.
//

#include "testing/standalone_test.h"
#include "seahash_simd.h"

namespace h = jumanpp::util::hashing;
using namespace jumanpp;

#ifdef JPP_SSE_IMPL
TEST_CASE("sse2 fasthash is identical to naive") {
  u64 data[] = {50, 80};
  h::FastHash1 naive;
  u64 r1 = naive.mix(data).result();
  u64 r2 = naive.mix(data + 1).result();
  u64 vresult[2];
  h::FastHash2 sse;
  sse.mix(data).store(vresult);
  CHECK(vresult[0] == r1);
  CHECK(vresult[1] == r2);
}

TEST_CASE("sse2 fasthash mask-store (1) is identical to naive") {
  u64 data[] = {50, 80};
  h::FastHash1 naive;
  u64 r1 = naive.mix(data).result();
  u64 r2 = naive.mix(data + 1).result();
  u32 result[4];
  h::FastHash2 sse;
  auto mask = (1u << 31u) - 1;
  auto state2 = sse.mix(data);
  state2.jointMaskStore(h::FastHash2{}, mask, result);
  CHECK(result[0] == (r1 & mask));
  CHECK(result[1] == (r2 & mask));
}

#endif

#ifdef JPP_AVX2
TEST_CASE("avx fasthash is identical to naive") {
  u64 data[] = {50, 80, 105, 241};
  h::FastHash1 naive;
  u64 r1 = naive.mix(data).result();
  u64 r2 = naive.mix(data + 1).result();
  u64 r3 = naive.mix(data + 2).result();
  u64 r4 = naive.mix(data + 3).result();
  u64 vresult[4];
  h::FastHash4 sse;
  sse.mix(data).store(vresult);
  CHECK(vresult[0] == r1);
  CHECK(vresult[1] == r2);
  CHECK(vresult[2] == r3);
  CHECK(vresult[3] == r4);
}

TEST_CASE("avx fasthash compact-merge works") {
  u64 data[] = {50, 80, 105, 241, 512, 321, 8, 1};
  h::FastHash1 naive;
  u64 r1 = naive.mix(data).result();
  u64 r2 = naive.mix(data + 1).result();
  u64 r3 = naive.mix(data + 2).result();
  u64 r4 = naive.mix(data + 3).result();
  u64 r5 = naive.mix(data + 4).result();
  u64 r6 = naive.mix(data + 5).result();
  u64 r7 = naive.mix(data + 6).result();
  u64 r8 = naive.mix(data + 7).result();
  u32 vresult[8];
  auto mask = (1u << 31u) - 1;
  h::FastHash4 a1;
  h::FastHash4 a2;
  auto z1 = a1.mix(data + 0);
  auto t1 = z1.maskCompact2(mask);
  auto z2 = a2.mix(data + 4);
  auto t2 = z2.maskCompact2(mask);
  auto t3 = t1.merge(t2);
  t3.store(vresult);
  CHECK(vresult[0] == (r1 & mask));
  CHECK(vresult[2] == (r2 & mask));
  CHECK(vresult[4] == (r3 & mask));
  CHECK(vresult[6] == (r4 & mask));
  CHECK(vresult[1] == (r5 & mask));
  CHECK(vresult[3] == (r6 & mask));
  CHECK(vresult[5] == (r7 & mask));
  CHECK(vresult[7] == (r8 & mask));
}
#endif
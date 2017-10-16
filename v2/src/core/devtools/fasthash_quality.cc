//
// Created by Arseny Tolmachev on 2017/10/16.
//

#include <iostream>
#include <random>
#include "util/fast_hash.h"
#include "util/flatmap.h"
#include "util/inlined_vector.h"
#include "util/types.hpp"

using namespace jumanpp;

struct Trigram {
  u64 t0;
  u64 t1;
  u64 t2;

  explicit Trigram(u64 a1, u64 a2 = ~0, u64 a3 = ~0) : t0{a1}, t1{a2}, t2{a3} {}
};

using h = jumanpp::util::hashing::FastHash1;

int main(int argc, char* argv[]) {
  u64 maxUni = 20 * 1000 * 1000;
  u64 maxBi = 100 * 1000;
  u64 maxTri = 5 * 1000;

  util::FlatMap<u64, util::InlinedVector<Trigram, 1>> data;

  auto apply = [&](u64 hc, Trigram tg) {
    auto& v = data[hc];
    v.push_back(tg);
  };

  for (u64 i = 0; i < maxUni; ++i) {
    auto hv = h{0xdead}.mix(i).result();
    apply(hv, Trigram{i});
  }

  for (u64 i = 0; i < maxBi; ++i) {
    auto p = h{0xbeef}.mix(i);
    for (u64 j = 0; j < maxBi; ++j) {
      auto hv = p.mix(j).result();
      apply(hv, Trigram{i, j});
    }
  }

  for (u64 i = 0; i < maxTri; ++i) {
    auto p1 = h{0xbabe}.mix(i);
    for (u64 j = 0; j < maxTri; ++j) {
      auto p2 = p1.mix(j);
      for (u64 k = 0; k < maxTri; ++k) {
        auto hv = p2.mix(k).result();
        apply(hv, Trigram{i, j, k});
      }
    }
  }

  auto total = maxUni + maxBi * maxBi + maxTri * maxTri * maxTri;
  std::cout << data.size() << "/" << total << " " << (total - data.size())
            << " collisions";

  std::minstd_rand rng{1};
}
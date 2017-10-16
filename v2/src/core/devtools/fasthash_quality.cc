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
  u32 t0;
  u32 t1;
  u32 t2;

  explicit Trigram(u32 a1, u32 a2 = ~0, u32 a3 = ~0) : t0{a1}, t1{a2}, t2{a3} {}
};

using h = jumanpp::util::hashing::FastHash1;

int main(int argc, char* argv[]) {
  u64 maxUni = 100 * 1000 * 1000;
  u64 maxBi = 50 * 1000;
  u64 maxTri = 3 * 1000;

  util::FlatMap<u64, util::InlinedVector<Trigram, 1>> data;

  auto apply = [&](u64 hc, Trigram tg) {
    auto& v = data[hc];
    v.push_back(tg);
  };

  for (u32 i = 0; i < maxUni; ++i) {
    auto hv = h{0xdead}.mix(i).result();
    apply(hv, Trigram{i});
  }

  std::cout << "Sequential!\n";
  std::cout << "Uni" << data.size() << "/" << maxUni << " "
            << (maxUni - data.size()) << " collisions\n";

  for (u32 i = 0; i < maxBi; ++i) {
    auto p = h{0xbeef}.mix(i);
    for (u32 j = 0; j < maxBi; ++j) {
      auto hv = p.mix(j).result();
      apply(hv, Trigram{i, j});
    }
  }

  auto biTotal = maxUni + maxBi * maxBi;
  std::cout << "+Uni" << data.size() << "/" << biTotal << " "
            << (biTotal - data.size()) << " collisions\n";

  for (u32 i = 0; i < maxTri; ++i) {
    auto p1 = h{0xbabe}.mix(i);
    for (u32 j = 0; j < maxTri; ++j) {
      auto p2 = p1.mix(j);
      for (u32 k = 0; k < maxTri; ++k) {
        auto hv = p2.mix(k).result();
        apply(hv, Trigram{i, j, k});
      }
    }
  }

  auto total = biTotal + maxTri * maxTri * maxTri;
  std::cout << "+Tri" << data.size() << "/" << total << " "
            << (total - data.size()) << " collisions\n";

  data.clear_no_resize();
  std::minstd_rand rng{1};
  std::uniform_int_distribution<u32> dst;

  for (u32 i = 0; i < maxUni; ++i) {
    auto hv = h{0xdead}.mix(dst(rng)).result();
    apply(hv, Trigram{i});
  }

  std::cout << "Random!\n";
  std::cout << "Uni" << data.size() << "/" << maxUni << " "
            << (maxUni - data.size()) << " collisions\n";

  for (u32 i = 0; i < maxBi; ++i) {
    auto p = h{0xbeef}.mix(dst(rng));
    for (u32 j = 0; j < maxBi; ++j) {
      auto hv = p.mix(dst(rng)).result();
      apply(hv, Trigram{i, j});
    }
  }

  std::cout << "+Uni" << data.size() << "/" << biTotal << " "
            << (biTotal - data.size()) << " collisions\n";

  for (u32 i = 0; i < maxTri; ++i) {
    auto p1 = h{0xbabe}.mix(dst(rng));
    for (u32 j = 0; j < maxTri; ++j) {
      auto p2 = p1.mix(dst(rng));
      for (u32 k = 0; k < maxTri; ++k) {
        auto hv = p2.mix(dst(rng)).result();
        apply(hv, Trigram{i, j, k});
      }
    }
  }

  std::cout << "+Tri" << data.size() << "/" << total << " "
            << (total - data.size()) << " collisions\n";
}
//
// Created by Arseny Tolmachev on 2017/10/16.
//

#include <array>
#include <iostream>
#include <random>
#include <unordered_map>
#include "util/array_slice.h"
#include "util/fast_hash.h"
#include "util/stl_util.h"
#include "util/string_piece.h"
#include "util/types.hpp"

using namespace jumanpp;

struct NgramInfo {
  u32 uniCnt = 0;
  u32 biCnt = 0;
  u32 triCnt = 0;
};

using h = jumanpp::util::hashing::FastHash1;

void doCheckLinear(i64 start, i64 add) {
  i64 maxUni = 10 * 1000 * 1000 * add + start;
  i64 maxBi = 20 * 1000 * add + start;
  i64 maxTri = 800 * add + start;
  auto biTotal = maxUni + maxBi * maxBi;
  auto total = biTotal + maxTri * maxTri * maxTri;

  std::unordered_map<u64, NgramInfo> data;
  std::array<u32, 256> b0;
  std::array<u32, 256> b1;
  std::array<u32, 256> b2;
  std::array<u32, 256> b3;
  util::fill(b0, 0);
  util::fill(b1, 0);
  util::fill(b2, 0);
  util::fill(b3, 0);

  std::cout << "checking [" << start << ", " << add << "]\n";

  auto apply = [&](u64 v) {
    auto bt0 = v & 0xff;
    b0[bt0] += 1;
    auto bt1 = (v >> 8) & 0xff;
    b1[bt1] += 1;
    auto bt2 = (v >> 16) & 0xff;
    b2[bt2] += 1;
    auto bt3 = (v >> 24) & 0xff;
    b3[bt3] += 1;
  };

  auto stats = [&](StringPiece name, util::ArraySlice<u32> arr) {
    u64 sum = 0;
    u32 min = std::numeric_limits<u32>::max();
    u32 max = 0;
    for (auto& i : arr) {
      sum += i;
      min = std::min(i, min);
      max = std::max(i, max);
    }

    auto avg = sum / arr.size();
    std::cout << "Stats for " << name << ": min=" << min << ", max=" << max
              << ", avg=" << avg << ", sum=" << sum << "\ndiff=[";
    sum = 0;
    min = std::numeric_limits<u32>::max();
    max = 0;
    for (auto& i : arr) {
      auto diff = static_cast<i64>(i) - static_cast<i64>(avg);
      auto absdiff = std::abs(diff);
      std::cout << absdiff << ",";
      sum += absdiff;
      min = std::min<u32>(min, absdiff);
      max = std::max<u32>(max, absdiff);
    }

    std::cout << "]";
    auto partavg = sum / arr.size();
    std::cout << "\nDiff stats " << name << "[" << start << "," << add
              << "] : min=" << min << ", max=" << max << ", avg=" << partavg
              << ", sum=" << sum
              << ", ratio=" << static_cast<double>(partavg) / avg << "\n";
  };

  for (i64 i = start; i < maxUni; i += add) {
    auto hv = h{}.mix(0xdead).mix(i).result();
    // data[hv].uniCnt += 1;
    apply(hv);
  }

  std::cout << "Sequential!\n";
  std::cout << "Uni " << data.size() << "/" << maxUni << " "
            << (maxUni - data.size()) << " collisions\n";
  stats("byte-0", b0);
  stats("byte-1", b1);
  stats("byte-2", b2);
  stats("byte-3", b3);

  for (i64 i = start; i < maxBi; i += add) {
    auto p = h{}.mix(0xbeef).mix(i);
    for (i64 j = start; j < maxBi; j += add) {
      auto hv = p.mix(j).result();
      // data[hv].biCnt += 1;
      apply(hv);
    }
    auto ratio = maxBi / 100;
    if (i % ratio == 0) {
      std::cout << "\rBigrams: " << i / ratio << "% finished" << std::flush;
    }
  }

  std::cout << "\r+Bi " << data.size() << "/" << biTotal << " "
            << (biTotal - data.size()) << " collisions\n";
  stats("byte-0", b0);
  stats("byte-1", b1);
  stats("byte-2", b2);
  stats("byte-3", b3);

  for (i64 i = start; i < maxTri; i += add) {
    auto p1 = h{}.mix(0xbabe).mix(i);
    for (i64 j = start; j < maxTri; j += add) {
      auto p2 = p1.mix(j);
      for (i64 k = start; k < maxTri; k += add) {
        auto hv = p2.mix(k).result();
        // data[hv].triCnt += 1;
        apply(hv);
      }
    }
    auto ratio = maxTri / 100;
    if (i % ratio == 0) {
      std::cout << "\rTrigrams: " << i / ratio << "% finished" << std::flush;
    }
  }

  std::cout << "+Tri" << data.size() << "/" << total << " "
            << (total - data.size()) << " collisions\n";

  stats("byte-0", b0);
  stats("byte-1", b1);
  stats("byte-2", b2);
  stats("byte-3", b3);
}

void checkRand() {
  u64 maxUni = 100 * 1000 * 1000;
  u64 maxBi = 50 * 1000;
  u64 maxTri = 3 * 1000;
  auto biTotal = maxUni + maxBi * maxBi;
  auto total = biTotal + maxTri * maxTri * maxTri;

  std::unordered_map<u64, NgramInfo> data;
  std::array<u32, 256> b0;
  std::array<u32, 256> b1;
  std::array<u32, 256> b2;
  std::array<u32, 256> b3;
  util::fill(b0, 0);
  util::fill(b1, 0);
  util::fill(b2, 0);
  util::fill(b3, 0);

  std::cout << "asking for " << total * 2 << " elements in a hashmap\n";
  data.reserve(total * 2);

  auto apply = [&](u64 v) {
    auto bt0 = v & 0xff;
    b0[bt0] += 1;
    auto bt1 = (v >> 8) & 0xff;
    b1[bt1] += 1;
    auto bt2 = (v >> 16) & 0xff;
    b2[bt2] += 1;
    auto bt3 = (v >> 24) & 0xff;
    b3[bt3] += 1;
  };

  auto stats = [](StringPiece name, util::ArraySlice<u32> arr) {
    u64 sum = 0;
    u32 min = std::numeric_limits<u32>::max();
    u32 max = 0;
    for (auto& i : arr) {
      sum += i;
      min = std::min(i, min);
      max = std::max(i, max);
    }

    auto avg = sum / arr.size();
    std::cout << "Stats for " << name << ": min=" << min << ", max=" << max
              << ", avg=" << avg << ", sum=" << sum << "\ndiff=[";
    sum = 0;
    min = std::numeric_limits<u32>::max();
    max = 0;
    for (auto& i : arr) {
      auto diff = static_cast<i64>(i) - static_cast<i64>(avg);
      auto absdiff = std::abs(diff);
      std::cout << absdiff << ",";
      sum += absdiff;
      min = std::min<u32>(min, absdiff);
      max = std::max<u32>(max, absdiff);
    }
    std::cout << "]\n";
    avg = sum / arr.size();
    std::cout << "Diff stats: min=" << min << ", max=" << max << ", avg=" << avg
              << ", sum=" << sum << "\ndiff=[";
  };

  std::minstd_rand rng{1};
  std::uniform_int_distribution<u32> dst;

  for (u32 i = 0; i < maxUni; ++i) {
    auto hv = h{}.mix(0xdead).mix(dst(rng)).result();
    data[hv].uniCnt += 1;
  }

  std::cout << "Random!\n";
  std::cout << "Uni " << data.size() << "/" << maxUni << " "
            << (maxUni - data.size()) << " collisions\n";
  stats("byte-0", b0);
  stats("byte-1", b1);
  stats("byte-2", b2);
  stats("byte-3", b3);

  for (u32 i = 0; i < maxBi; ++i) {
    auto p = h{}.mix(0xbeef).mix(dst(rng));
    for (u32 j = 0; j < maxBi; ++j) {
      auto hv = p.mix(dst(rng)).result();
      data[hv].biCnt += 1;
    }
    auto ratio = maxBi / 100;
    if (i % ratio == 0) {
      std::cout << "\rBigrams: " << i / ratio << "% finished" << std::flush;
    }
  }

  std::cout << "\r+Bi " << data.size() << "/" << biTotal << " "
            << (biTotal - data.size()) << " collisions\n";
  stats("byte-0", b0);
  stats("byte-1", b1);
  stats("byte-2", b2);
  stats("byte-3", b3);

  for (u32 i = 0; i < maxTri; ++i) {
    auto p1 = h{}.mix(0xbabe).mix(dst(rng));
    for (u32 j = 0; j < maxTri; ++j) {
      auto p2 = p1.mix(dst(rng));
      for (u32 k = 0; k < maxTri; ++k) {
        auto hv = p2.mix(dst(rng)).result();
        data[hv].triCnt += 1;
      }
    }
    auto ratio = maxTri / 100;
    if (i % ratio == 0) {
      std::cout << "\rTrigrams: " << i / ratio << "% finished" << std::flush;
    }
  }

  std::cout << "\r+Tri " << data.size() << "/" << total << " "
            << (total - data.size()) << " collisions\n";
  stats("byte-0", b0);
  stats("byte-1", b1);
  stats("byte-2", b2);
  stats("byte-3", b3);
}

int main(int argc, char* argv[]) {
  doCheckLinear(0, 1);
  doCheckLinear(0, 2);
  doCheckLinear(1, 2);
  doCheckLinear(0, 3);
  doCheckLinear(0, 4);
  doCheckLinear(0, 8);
}
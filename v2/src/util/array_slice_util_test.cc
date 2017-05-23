#include "array_slice_util.h"
#include "testing/standalone_test.h"

#include "core/analysis/lattice_builder.h"

TEST_CASE("compact works with ints") {
  SECTION("when erasing first element") {
    std::vector<int> data{1, 5, 8, 12, 31};
    std::vector<int> indices{0};
    jumanpp::util::compact(data, indices);
    CHECK(data[0] == 5);
    CHECK(data[1] == 8);
    CHECK(data[2] == 12);
    CHECK(data[3] == 31);
  }

  SECTION("when erasing two elements") {
    std::vector<int> data{1, 5, 8, 12, 31};
    std::vector<int> indices{0, 3};
    jumanpp::util::compact(data, indices);
    CHECK(data[0] == 5);
    CHECK(data[1] == 8);
    CHECK(data[2] == 31);
  }

  SECTION("when erasing first and last elements") {
    std::vector<int> data{1, 5, 8, 12, 31};
    std::vector<int> indices{0, 4};
    jumanpp::util::compact(data, indices);
    CHECK(data[0] == 5);
    CHECK(data[1] == 8);
    CHECK(data[2] == 12);
  }

  SECTION("when erasing three elements") {
    std::vector<int> data{1, 5, 8, 12, 31};
    std::vector<int> indices{0, 1, 4};
    jumanpp::util::compact(data, indices);
    CHECK(data[0] == 8);
    CHECK(data[1] == 12);
  }
}

using A = jumanpp::core::analysis::LatticeNodeSeed;

A a(short i) { return A{jumanpp::core::EntryPtr{i}, i, i}; }

TEST_CASE("compact works with large lists (4,8)") {
  std::vector<A> data{a(1), a(2),  a(3),  a(4),  a(5),  a(6),  a(7),  a(8),
                      a(9), a(10), a(11), a(12), a(13), a(14), a(15), a(16)};
  std::vector<int> indices{4, 8};
  jumanpp::util::compact(data, indices);
  std::vector<A> data2{a(1),  a(2),  a(3),  a(4),  a(6),  a(7),  a(8), a(10),
                       a(11), a(12), a(13), a(14), a(15), a(16), a(9), a(5)};
  CHECK(data == data2);
}

TEST_CASE("compact works with large lists (3)") {
  std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  std::vector<int> indices{3};
  jumanpp::util::compact(data, indices);
  std::vector<int> data2{1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 4};
  CHECK(data.size() == 16);
  CHECK(data == data2);
}
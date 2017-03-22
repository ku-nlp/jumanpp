#include "array_slice_util.h"
#include "testing/standalone_test.h"

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
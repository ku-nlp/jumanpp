//
// Created by Arseny Tolmachev on 2017/03/04.
//

#include <limits>
#include "util/serialization.h"
#include "testing/standalone_test.h"
#include "util/serialization_flatmap.h"
#include <limits>

using namespace jumanpp;
using namespace jumanpp::util::serialization;

struct Test {
  i32 a;
};

template <typename Arch>
void Serialize(Arch& a, Test& o) {
  a& o.a;
}

enum class TestClass { A, B, C };

SERIALIZE_ENUM_CLASS(TestClass, int)

TEST_CASE("serialization works with small object") {
  Test a = {5};
  Saver s;
  s.save(a);
  Loader l{s.result()};
  Test b;
  CHECK(l.load(&b));
  CHECK(a.a == b.a);
}

TEST_CASE("serialization works with vector of strings") {
  std::vector<std::string> data{"a", "b", "c"};
  Saver s;
  s.save(data);
  Loader l{s.result()};
  std::vector<std::string> d2;
  CHECK(l.load(&d2));
  CHECK(d2.size() == 3);
  CHECK(d2[0] == "a");
  CHECK(d2[1] == "b");
  CHECK(d2[2] == "c");
}

TEST_CASE("serialization works with enum classes") {
  auto data = TestClass::C;
  Saver s;
  s.save(data);
  Loader l{s.result()};
  TestClass d2 = TestClass::A;
  CHECK(l.load(&d2));
  CHECK(d2 == TestClass::C);
}

TEST_CASE("serialization works with floats") {
  auto check = [](float v) {
    CAPTURE(v);
    Saver s;
    s.save(v);
    float v2 = 0;
    Loader l{s.result()};
    REQUIRE(l.load(&v2));
    return v2;
  };

  auto nan1 = std::numeric_limits<float>::quiet_NaN();
  // CHECK(std::isnan(check(nan1)));
  CHECK(check(0.0f) == 0.0f);
  CHECK(check(1.0f) == 1.0f);
  CHECK(check(-1.0f) == -1.0f);
  auto inf = std::numeric_limits<float>::infinity();
  CHECK(check(inf) == inf);
}

TEST_CASE("serialization works with a flatmap") {
  util::FlatMap<StringPiece, i32> map1;
  map1["a"] = 1;
  map1["b"] = 2;
  map1["c"] = 3;
  Saver s;
  s.save(map1);
  util::FlatMap<StringPiece, i32> map2;
  Loader l{s.result()};
  CHECK(l.load(&map2));
  CHECK(map1.size() == map2.size());
  CHECK(map1["a"] == map2["a"]);
  CHECK(map1["b"] == map2["b"]);
  CHECK(map1["c"] == map2["c"]);
}

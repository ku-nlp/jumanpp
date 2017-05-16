//
// Created by Arseny Tolmachev on 2017/05/10.
//

#include "lazy.h"
#include "testing/standalone_test.h"

using namespace jumanpp::util;

template <typename T>
class Capturer {
  T& ref;
  T val;

 public:
  Capturer(T& r, T v) : ref{r}, val{v} {}
  ~Capturer() { ref = val; }
};

template <typename T>
class Adder {
  T* ref;
  T val;

 public:
  Adder(T& r, T v) : ref{&r}, val{v} {}
  ~Adder() { *ref += val; }
};

TEST_CASE("lazy works when initialized") {
  int i = 2;
  {
    Lazy<Capturer<int>> lc;
    lc.initialize(i, 5);
    CHECK(i == 2);
  }
  CHECK(i == 5);
}

TEST_CASE("lazy works when not initialized") {
  int i = 2;
  {
    Lazy<Capturer<int>> lc;
    CHECK(i == 2);
  }
  CHECK(i == 2);
}

TEST_CASE("lazy works with double construction") {
  int i = 2;
  {
    Lazy<Adder<int>> lc1;
    lc1.initialize(i, 3);
    CHECK(i == 2);
    lc1.initialize(i, 4);
    CHECK(i == 5);
  }
  CHECK(i == 9);
}

TEST_CASE("lazy works when moved") {
  int i = 2;
  {
    Lazy<Adder<int>> lc1;
    lc1.initialize(i, 3);
    Lazy<Adder<int>> lc2;
    lc2.initialize(i, 4);
    lc1 = std::move(lc2);
    CHECK(i == 2);
  }
  CHECK(i == 6);
}
#include <catch.hpp>
#include <numeric>
#include <util/fixed_queue.hpp>

TEST_CASE("Queue works with one element", "[queue]") {
  jumanpp::fixed_queue<int> q(5);
  q.enqueue(5);
  REQUIRE(q.peek() == 5);
}

TEST_CASE("Queue works with two elements", "[queue]") {
  jumanpp::fixed_queue<int> q(5);
  q.enqueue(5);
  q.enqueue(6);
  REQUIRE(q.peek() == 6);
}

TEST_CASE("Queue works with five elements", "[queue]") {
  jumanpp::fixed_queue<int, std::greater<int>> q(3);
  REQUIRE(q.size() == 0);
  REQUIRE(!q.enqueue(5));
  REQUIRE(q.size() == 1);
  REQUIRE(!q.enqueue(6));
  REQUIRE(q.size() == 2);
  REQUIRE(!q.enqueue(7));
  REQUIRE(q.size() == 3);
  int val = 0;
  REQUIRE(q.enqueue(8, &val));
  REQUIRE(q.size() == 3);
  REQUIRE(val == 5);
  REQUIRE(q.enqueue(9, &val));
  REQUIRE(q.size() == 3);
  REQUIRE(val == 6);
  REQUIRE(q.peek() == 7);
}

TEST_CASE("Queue works with floats", "[queue]") {
  jumanpp::fixed_queue<float, std::greater<float>> q(4);
  q.enqueue(0.1f);
  q.enqueue(0.7f);
  q.enqueue(0.2f);
  q.enqueue(0.8f);
  q.enqueue(0.4f);
  q.enqueue(0.0f);
  q.enqueue(0.9f);
  REQUIRE(q.size() == 4);
  REQUIRE(q.peek() == 0.4f);
}
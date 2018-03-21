//
// Created by Arseny Tolmachev on 2017/10/05.
//

#include <condition_variable>
#include <mutex>
#include <vector>
#include "common.hpp"
#include "logging.hpp"

#ifndef JUMANPP_BOUNDED_QUEUE_H
#define JUMANPP_BOUNDED_QUEUE_H

namespace jumanpp {
namespace util {

template <typename T>
class bounded_queue {
  std::vector<T> items_;
  unsigned int head_ = 0;
  unsigned int tail_ = 0;
  unsigned int capa_;
  std::mutex mutex_;
  std::condition_variable cv_;

  using lock_t = std::unique_lock<std::mutex>;

 public:
  size_t size() const noexcept {
    auto res = head_ - tail_;
    JPP_DCHECK_LE(res, capa_);
    return res;
  }

  size_t remaining() const noexcept { return capa_ - size(); }

  bounded_queue() : capa_{0} {}
  explicit bounded_queue(unsigned int max_size)
      : items_(max_size), capa_{max_size} {}

  void initialize(unsigned int max_size) {
    items_.resize(max_size);
    capa_ = max_size;
  }

  bool offer(T&& item, unsigned int reserve = 0) noexcept(std::is_nothrow_move_assignable<T>::value) {
    lock_t lock(mutex_);
    if (remaining() <= reserve) {
      return false;
    }
    items_[head_ % capa_] = std::move(item);
    head_ += 1;
    cv_.notify_one();
    return true;
  }

  // an item will be MOVED there
  bool recieve(T* result) noexcept(std::is_nothrow_move_assignable<T>::value) {
    lock_t lock(mutex_);
    if (size() == 0) {
      return false;
    }
    *result = std::move(items_[tail_ % capa_]);
    tail_ += 1;
    return true;
  }

  T waitFor() noexcept(std::is_nothrow_move_constructible<T>::value) {
    lock_t lock(mutex_);
    while (size() == 0) {
      cv_.wait(lock);
    }
    auto pos = tail_ % capa_;
    tail_ += 1;
    return std::move(items_[pos]);
  }

  void unblock_all() { cv_.notify_all(); }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_BOUNDED_QUEUE_H

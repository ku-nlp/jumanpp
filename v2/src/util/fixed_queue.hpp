//
// Created by Arseny Tolmachev on 2016/12/28.
//

#ifndef JUMANPP_FIXED_QUEUE_HPP
#define JUMANPP_FIXED_QUEUE_HPP

#include <algorithm>
#include <vector>

namespace jumanpp {

/***
 *
 * This class wraps std::vector and stl heap functions to build min-queue of
 * fixed size.
 *
 * @tparam T element type
 * @tparam Comp comparator
 * @tparam Alloc allocator
 */
template <typename T, typename Comp = std::less<T>,
          typename Alloc = std::allocator<T>>
class fixed_queue {
  std::vector<T, Alloc> data_;
  Comp cmp_;

  void insert(const T &input) {
    data_.emplace_back(input);
    std::push_heap(data_.begin(), data_.end(), Comp());
  }

  bool replace(const T &input, T *return_slot) {
    Comp cmp;
    if (cmp(peek(), input)) {
      return false;
    }

    std::pop_heap(data_.begin(), data_.end(), cmp);

    auto last_elem = data_.end() - 1;

    if (return_slot != nullptr) {
      *return_slot = *last_elem;
    }

    *last_elem = input;

    std::push_heap(data_.begin(), data_.end(), cmp);

    return true;
  }

 public:
  /**
   * Construct the queue with specified maximum size
   * @param max_size
   * @param cmp
   * @param alloc
   */
  fixed_queue(size_t max_size, Comp cmp = Comp(), Alloc alloc = Alloc())
      : data_{alloc}, cmp_{cmp} {
    data_.reserve(max_size);
  }

  /**
   * Enqueue an element in the queue with possible overflow.
   * @param input element to add
   * @param returned pointer to write the discarded element into
   * @return true if an element was discarded, false otherwise
   */
  bool enqueue(const T &input, T *returned = nullptr) {
    if (data_.size() < data_.capacity()) {
      insert(input);
      return false;
    } else {
      return replace(input, returned);
    }
  }

  /**
   *
   * @return max element in this queue. Non-const version.
   */
  T &peek() { return data_[0]; }

  /**
   *
   * @return max element in this queue
   */
  const T &peek() const { return data_[0]; }

  /**
   *
   * @return number of elements in this queue
   */
  size_t size() const { return std::min(data_.capacity(), data_.size()); }
};
}

#endif  // JUMANPP_FIXED_QUEUE_HPP

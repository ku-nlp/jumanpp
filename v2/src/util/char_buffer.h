//
// Created by Arseny Tolmachev on 2017/03/04.
//

#ifndef JUMANPP_CHAR_BUFFER_H
#define JUMANPP_CHAR_BUFFER_H

#include <array>
#include <vector>
#include "array_slice.h"
#include "string_piece.h"

namespace jumanpp {
namespace util {

/**
 * This class can intern StringPiece and allow to share them
 * as long as CharBuffer which created them lives.
 *
 * CharBuffer allocates memory in pages and sequentially fills it with
 * StrinPieces
 *
 * @tparam PageSize page size, 1M default
 */
template <size_t PageSize = (1 << 20)>  // 1M by default
class CharBuffer {
  static constexpr ptrdiff_t page_size = PageSize;
  using page = std::array<char, page_size>;
  std::vector<page*> storage_;

  page* current_ = nullptr;
  ptrdiff_t position_ = page_size;
  ptrdiff_t npage = -1;

  bool ensure_size(size_t sz) {
    if (sz > page_size) {
      return false;
    }
    ptrdiff_t remaining = page_size - position_;
    if (remaining < sz) {
      ++npage;
      if (npage < storage_.size()) {
        current_ = storage_[npage];
      } else {
        auto ptr = new page;
        current_ = ptr;
        storage_.push_back(ptr);
      }
      position_ = 0;
    }

    return true;
  }

 public:
  ~CharBuffer() {
    for (auto p : storage_) {
      delete p;
    }
  }

  bool prepareMemory(StringPiece sp, util::MutableArraySlice<char>* res) {
    auto psize = sp.size();
    JPP_RET_CHECK(ensure_size(sp.size()));
    auto begin = current_->data() + position_;
    *res = util::MutableArraySlice<char>{begin, psize};
    position_ += psize;
    return true;
  }

  bool importMutable(StringPiece sp, util::MutableArraySlice<char>* res) {
    JPP_RET_CHECK(prepareMemory(sp, res));
    std::copy(sp.char_begin(), sp.char_end(), res->begin());
    return true;
  }

  bool import(StringPiece* sp) {
    util::MutableArraySlice<char> as;
    JPP_RET_CHECK(importMutable(*sp, &as));
    *sp = StringPiece{as.begin(), as.end()};
    return true;
  }

  void reset() {
    npage = -1;
    position_ = page_size;
  }
};

}  // namespace util
}  // namespace jumanpp

#endif  // JUMANPP_CHAR_BUFFER_H

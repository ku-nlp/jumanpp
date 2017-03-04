//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "csv_reader.h"
#include "stl_util.h"

namespace jumanpp {
namespace util {

CsvReader::CsvReader(char separator_, char quote_)
    : separator_(separator_), quote_{quote_} {}

Status CsvReader::open(const StringPiece &filename) {
  JPP_RETURN_IF_ERROR(file_.open(filename, MMapType::ReadOnly));
  JPP_RETURN_IF_ERROR(file_.map(&fragment_, 0, file_.size()));
  return initFromMemory(fragment_.asStringPiece());
}

bool CsvReader::nextLine() {
  auto end = end_;
  auto position = position_;
  if (position == end) return false;
  temp_.reset();
  fields_.clear();
  line_number_ += 1;

  auto field_start = position;
  bool shouldEmit = true;

  for (;; ++position) {
    auto ch = *position;
    if (ch == '\n') {
      if (shouldEmit) {
        fields_.emplace_back(field_start, position);
      }
      // next call starts from the next string, skipping EOL
      position_ = position + 1;
      // handle \n\r pattern
      if (position_ < end && *position_ == '\r') {
        ++position_;
      }
      break;
    }

    // handle \r\n pattern
    if (ch == '\r') {
      auto next = position + 1;
      if (next < end && *next == '\n') {
        if (shouldEmit) {
          fields_.emplace_back(field_start, position);
        }
        position_ = next + 1;
        break;
      }
    }

    if (ch == separator_) {
      if (shouldEmit) {
        fields_.emplace_back(field_start, position);
      }
      field_start = position + 1;
      shouldEmit = true;
      continue;
    }

    if (ch == quote_) {
      // begin quote can be only at field start
      if (position != field_start) {
        return false;
      }
      StringPiece result{position, end};
      bool containsEscapes = handleQuote(position + 1, &result);
      auto resEnd = result.char_end();
      if (resEnd == end) {  // there wasn't quote end present
        return false;
      }
      // this must point to quote_ then
      JPP_DCHECK_EQ(*resEnd, quote_);
      auto next = resEnd + 1;
      // and if the field does not end there, csv is ill-formed
      if (next != end && !util::contains({'\n', '\r', separator_}, *next)) {
        return false;
      }

      // otherwise emit the result
      if (containsEscapes) {
        StringPiece unescaped;
        JPP_RET_CHECK(unescapeString(result, &unescaped));
        fields_.emplace_back(unescaped);
      } else {
        fields_.emplace_back(result);
      }
      shouldEmit = false;
      position = resEnd;
    }

    // This block executes when end of file is reached
    if (position == end) {
      if (field_start != position) {
        if (shouldEmit) {
          fields_.emplace_back(field_start, position);
        }
        position_ = position;
      }
      break;
    }
  }

  return !fields_.empty();
}

i32 CsvReader::numFields() const { return static_cast<i32>(fields_.size()); }

StringPiece CsvReader::field(i32 idx) const {
  JPP_DCHECK_IN(idx, 0, fields_.size());
  return fields_[idx];
}

Status CsvReader::initFromMemory(const StringPiece &data) {
  position_ = data.char_begin();
  end_ = data.char_end();
  line_number_ = 0;
  return Status::Ok();
}

bool CsvReader::unescapeString(StringPiece sp, StringPiece *result) {
  auto source = sp.char_begin();
  auto end = sp.char_end();
  // find first instance of escape
  while (source < end && *source != quote_) {
    ++source;
  }

  // wtf case, should not happen
  if ((source + 1) == end) {
    return false;
  }

  // corner case: quote is the last part of the string
  if ((source + 2) == end) {
    *result = StringPiece{sp.char_begin(), source + 1};
    return true;
  }

  source += 1;

  MutableArraySlice<char> data;
  JPP_RET_CHECK(temp_.prepareMemory(sp, &data));
  std::copy(sp.char_begin(), source, data.begin());
  auto target = data.begin() + (source - sp.char_begin());
  source += 1;

  // main cycle: copy string, while skipping escaped quotes
  while (source != end) {
    if (*source == quote_) {
      ++source;
      if (source == end) {
        return false;
      }
      if (*source != quote_) {
        return false;
      }
    }

    *target = *source;
    ++target;
    ++source;
  }
  *result = StringPiece{data.begin(), target};
  return true;
}

bool CsvReader::handleQuote(const char *position, StringPiece *result) {
  auto start = position;
  auto end = end_;
  bool wasQuoted = false;
  while (position != end) {
    char ch = *position;
    if (ch == quote_) {
      ++position;
      if (position == end) {  // first case: quote followed by EOS
        *result = StringPiece{start, position - 1};
        return wasQuoted;
      }

      char nextCh = *position;
      if (nextCh == quote_) {
        wasQuoted = true;
        ++position;
        continue;
      } else {
        *result = StringPiece{start, position - 1};
        return wasQuoted;
      }
    }
    ++position;
  }
  return false;
}
}
}

//
// Created by Arseny Tolmachev on 2017/03/03.
//

#ifndef JUMANPP_FEATURE_IMPL_COMPUTE_H
#define JUMANPP_FEATURE_IMPL_COMPUTE_H

#include "core/features_api.h"
#include "core/impl/feature_impl_types.h"
#include "core/impl/feature_types.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

class ComputeFeatureContext {};

inline u64 hashIndexed(u64 seed, util::ArraySlice<u64> input,
                       util::ArraySlice<i32> indices) {
  return 0;
}

constexpr size_t MaxQueryElements = 64;

enum class ComareResult { Lesser, Equal, Greater };

class MatchTupleComputeFeatureImpl {
  i32 featureIdx;

  // indices of fields and features of question part of this feature
  util::ArraySlice<i32> references;

  // this is assumed to contain tuples of field pointers
  // there are in total matchData.size() / references.size() tuples
  util::ArraySlice<i32> matchData;

  // feature indices of true branch
  util::ArraySlice<i32> trueBranch;
  // feature indices of fals branch
  util::ArraySlice<i32> falseBranch;

  size_t dataEntries;

 public:
  MatchTupleComputeFeatureImpl() {}

  template <size_t S1, size_t S2, size_t S3, size_t S4>
  constexpr MatchTupleComputeFeatureImpl(i32 featureIdx,
                                         const i32 (&refs)[S1],
                                         const i32 (&data)[S2], const i32 (&trueB)[S3],
                                         const i32 (&falseB)[S4])
      : featureIdx{featureIdx},
        references{refs},
        matchData{data},
        trueBranch{trueB},
        falseBranch{falseB},
        dataEntries{matchData.size() / references.size()} {
    JPP_DCHECK_GT(dataEntries, 0);
    JPP_DCHECK((matchData.size() % dataEntries == 0) &&
               "matchData should have correct number of entries");
  }

  inline static ComareResult compareSameSize(util::ArraySlice<i32> left,
                                             util::ArraySlice<i32> right) {
    for (int i = 0; i < left.size(); ++i) {
      auto l = left[i];
      auto r = right[i];
      if (l < r) {
        return ComareResult::Lesser;
      } else if (l > r) {
        return ComareResult::Greater;
      }
    }
    return ComareResult::Equal;
  }

  inline bool matches(const util::ArraySlice<i32>& entry) const noexcept {
    JPP_DCHECK_LT(dataEntries, MaxQueryElements);
    JPP_DCHECK_EQ(matchData.size() % dataEntries, 0);
    i32 buffer[MaxQueryElements];

    size_t querySz = references.size();

    for (int i = 0; i < querySz; ++i) {
      buffer[i] = entry.at(references[i]);
    }

    util::ArraySlice<i32> query{buffer, querySz};

    size_t lowest = 0;
    size_t highest = dataEntries;

    // binary search
    while (true) {
      if (highest == lowest) {
        return false;
      }
      size_t mid = (lowest + highest) / 2;
      util::ArraySlice<i32> dataEntry{matchData, mid * querySz, querySz};
      auto cmp = compareSameSize(query, dataEntry);
      switch (cmp) {
        case ComareResult::Equal:
          return true;
        case ComareResult::Lesser:
          highest = mid;
          break;
        case ComareResult::Greater:
          lowest = mid + 1;
          break;
      }
    }
  }

  inline void apply(ComputeFeatureContext* ctx, EntryPtr entryPtr,
                    const util::ArraySlice<i32>& entry,
                    util::MutableArraySlice<u64>* features) const noexcept {
    if (matches(entry)) {
      features->at(featureIdx) = hashIndexed(featureIdx, *features, trueBranch);
    } else {
      features->at(featureIdx) =
          hashIndexed(featureIdx, *features, falseBranch);
    }
  }
};

}  // impl
}  // features
}  // core
}  // jumanpp

#endif  // JUMANPP_FEATURE_IMPL_COMPUTE_H

//
// Created by Arseny Tolmachev on 2017/11/01.
//

#ifndef JUMANPP_ENTRY_BUILDER_H
#define JUMANPP_ENTRY_BUILDER_H

#include "core/dic/darts_trie.h"
#include "core/dic/dic_feature_impl.h"
#include "core/dic/field_import.h"
#include "util/coded_io.h"
#include "util/csv_reader.h"
#include "util/flatmap.h"
#include "util/flatset.h"
#include "util/inlined_vector.h"
#include "util/stl_util.h"
#include "util/types.hpp"

namespace jumanpp {
namespace core {
namespace dic {

struct DicTrieBuilder {
  util::CodedBuffer entryPtrBuffer;
  util::FlatMap<i32, util::InlinedVector<i32, 4>> entriesWithField;
  DoubleArrayBuilder daBuilder;

  void addEntry(i32 fieldValue, i32 entryPtr) {
    entriesWithField[fieldValue].push_back(entryPtr);
  }

  Status buildTrie(const impl::StringStorage& strings);
};

struct EntryTableBuilder;

struct DicEntryData {
  xi::FeatureBuffer features;
  std::vector<xi::FeatureBuffer> data;

  void addData(EntryTableBuilder* bldr, const util::CsvReader& csv);
  i32 write(EntryTableBuilder* bldr);
};

struct DicRawEntry {
  util::FlatMap<xi::DicEntryFeatures, DicEntryData, xi::DicEntryFeaturesHash>
      data;
  u32 count = 0;
};

struct DicEntryBag {
  util::FlatMap<i32, DicRawEntry> entries;

  i32 add(EntryTableBuilder* bldr, const xi::FeatureBuffer& features,
          const util::CsvReader& csv);
  i32 write(EntryTableBuilder* bldr, const util::CsvReader& csv);
};

struct EntryTableBuilder {
  /**
   * This set contains row numbers which serve patterns for UNK builders
   */
  util::FlatMap<i32, i32> ignoredRows;
  util::CodedBuffer entryDataBuffer;
  DicTrieBuilder trieBuilder;
  impl::StringStorage surfaceCounter_;
  DicEntryBag entryBag;

  std::vector<std::unique_ptr<DicFeatureBase>> features_;
  std::vector<ImportFieldDicFeatureImpl> content_;
  std::vector<i32> dedupIdxes_;
  xi::FeatureBuffer featureBuf_;
  u32 surfaceIdx_;
  impl::StringStorage* realSurfaceStorage_;

  void computeFeatures(util::MutableArraySlice<i32> result,
                       const util::CsvReader& csv) const {
    util::fill(result, 0);
    for (auto& f : features_) {
      f->apply(result, csv);
    }
  }

  xi::DicEntryFeatures gather(const xi::FeatureBuffer& fb) const {
    xi::FeatureBuffer res;
    for (auto idx : dedupIdxes_) {
      res.push_back(fb[idx]);
    }
    return xi::DicEntryFeatures{res};
  }

  i32 surfaceOf(const util::CsvReader& csv) const {
    return realSurfaceStorage_->valueOf(csv.field(surfaceIdx_));
  }

  i32 importOneLine(std::vector<ColumnImportContext>& columns,
                    const util::CsvReader& csv);

  void addSurface(StringPiece surface) {
    surfaceCounter_.increaseFieldValueCount(surface);
  }

  Status createFeatures(const spec::FeaturesSpec& features,
                        const DicFeatureContext& ctx);
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_ENTRY_BUILDER_H

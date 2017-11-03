//
// Created by Arseny Tolmachev on 2017/11/01.
//

#ifndef JUMANPP_DIC_FEATURE_IMPL_H
#define JUMANPP_DIC_FEATURE_IMPL_H

#include "core/dic/field_import.h"
#include "core/impl/int_seq_util.h"
#include "core/spec/spec_types.h"
#include "util/array_slice.h"
#include "util/csv_reader.h"
#include "util/flatset.h"
#include "util/status.hpp"

namespace jumanpp {
namespace core {
namespace dic {

namespace xi = jumanpp::core::impl;

struct ColumnImportContext {
  i32 index;
  const spec::FieldDescriptor* descriptor;
  bool isTrieIndexed = false;
  std::unique_ptr<impl::FieldImporter> importer;

  Status initialize(i32 index, const spec::FieldDescriptor* descr,
                    std::vector<impl::StringStorage>& storages);

  bool importFieldValue(const util::CsvReader& reader) {
    return importer->importFieldValue(reader);
  }
};

struct DicFeatureContext {
  const std::vector<impl::StringStorage>& storage;
  const std::vector<ColumnImportContext>& importers;
  const std::vector<util::CodedBuffer>& stringBuffers;
  const std::vector<util::CodedBuffer>& intBuffers;
};

class DicFeatureBase {
 public:
  virtual ~DicFeatureBase() = default;
  virtual Status initialize(const DicFeatureContext& ctx,
                            const spec::DicImportDescriptor& desc) = 0;
  virtual void apply(util::MutableArraySlice<i32> featureData,
                     const util::CsvReader& csv) const = 0;
};

class ImportFieldDicFeatureImpl : public DicFeatureBase {
  impl::FieldImporter* importer_;
  u32 target_;

 public:
  Status initialize(const DicFeatureContext& ctx,
                    const spec::DicImportDescriptor& desc) override;
  void apply(util::MutableArraySlice<i32> featureData,
             const util::CsvReader& csv) const override {
    JPP_CAPTURE(csv.lineNumber());
    featureData.at(target_) = importer_->fieldPointer(csv);
  }
};

class MatchListDicFeatureImpl : public DicFeatureBase {
  const impl::StringStorage* storage_;
  i32 fieldIdx_;
  u32 target_;
  u32 shift_;
  char dataSep_;
  std::string kvSep_;
  util::FlatSet<i32> data_;

 public:
  Status initialize(const DicFeatureContext& ctx,
                    const spec::DicImportDescriptor& desc) override;
  void apply(util::MutableArraySlice<i32> featureData,
             const util::CsvReader& csv) const override;
};

class MatchFieldTupleDicFeatureImpl : public DicFeatureBase {
  std::vector<const impl::StringStorage*> storages_;
  std::vector<i32> fields_;
  u32 target_;
  u32 shift_;
  util::FlatSet<xi::DicEntryFeatures, xi::DicEntryFeaturesHash> data_;

 public:
  Status initialize(const DicFeatureContext& ctx,
                    const spec::DicImportDescriptor& desc) override;
  void apply(util::MutableArraySlice<i32> featureData,
             const util::CsvReader& csv) const override;
};

}  // namespace dic
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_DIC_FEATURE_IMPL_H

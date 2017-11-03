//
// Created by Arseny Tolmachev on 2017/11/01.
//

#include "entry_builder.h"
#include "dic_build_detail.h"

namespace jumanpp {
namespace core {
namespace dic {

Status DicTrieBuilder::buildTrie(const impl::StringStorage& strings) {
  for (auto& v : strings) {
    StringPiece key = v.first;
    i32 keyPtr = v.second;
    auto it = entriesWithField.find(keyPtr);
    if (it != entriesWithField.end()) {
      auto& entries = it->second;
      auto entriesPtr = static_cast<i32>(entryPtrBuffer.position());
      impl::writePtrsAsDeltas(entries, entryPtrBuffer);
      daBuilder.add(key, entriesPtr);
    }
  }
  return daBuilder.build();
}

i32 EntryTableBuilder::importOneLine(std::vector<ColumnImportContext>& columns,
                                     const util::CsvReader& csv) {
  computeFeatures(&featureBuf_, csv);
  auto cnt = entryBag.add(this, featureBuf_, csv);
  if (cnt == surfaceCounter_.valueOf(csv.field(surfaceIdx_))) {
    return entryBag.write(this, csv);
  }

  return 0;
}

Status EntryTableBuilder::createFeatures(const spec::FeaturesSpec& features,
                                         const DicFeatureContext& ctx) {
  auto createFeature = [&](const spec::DicImportDescriptor& f) -> Status {
    std::unique_ptr<DicFeatureBase> ptr;
    switch (f.kind) {
      case spec::DicImportKind::Invalid: {
        return JPPS_INVALID_PARAMETER << "feature was not initialized";
      }
      case spec::DicImportKind::ImportAsData: {
        content_.emplace_back();
        JPP_RETURN_IF_ERROR(content_.back().initialize(ctx, f));
        return Status::Ok();
      }
      case spec::DicImportKind::ImportAsFeature:
        ptr.reset(new ImportFieldDicFeatureImpl{});
        break;
      case spec::DicImportKind::MatchListKey:
        ptr.reset(new MatchListDicFeatureImpl{});
        break;
      case spec::DicImportKind::MatchFields:
        ptr.reset(new MatchFieldTupleDicFeatureImpl{});
        break;
    }
    JPP_RETURN_IF_ERROR(ptr->initialize(ctx, f));
    features_.push_back(std::move(ptr));
    return Status::Ok();
  };

  for (auto& f : features.dictionary) {
    JPP_RIE_MSG(createFeature(f), "for feature: " << f.name);
  }
  return Status::Ok();
}

i32 DicEntryBag::add(EntryTableBuilder* bldr, const xi::FeatureBuffer& features,
                     const util::CsvReader& csv) {
  if (bldr->ignoredRows.exists(csv.lineNumber())) {
    // just write it
    DicEntryData d;
    d.features = features;
    d.addData(bldr, csv);
    bldr->ignoredRows[csv.lineNumber()] = d.write(bldr);
    return 0;
  }

  auto surf = bldr->surfaceOf(csv);
  auto dedupFeature = bldr->gather(features);

  auto& obj = entries[surf];
  auto& v = obj.data[dedupFeature];
  if (v.features.empty()) {
    v.features = features;
  }
  v.addData(bldr, csv);
  obj.count += 1;

  return obj.count;
}

i32 DicEntryBag::write(EntryTableBuilder* bldr, const util::CsvReader& csv) {
  auto surf = bldr->surfaceOf(csv);
  auto it = entries.find(surf);

  for (auto p : it->second.data) {
    auto ptr = p.second.write(bldr);
    bldr->trieBuilder.addEntry(surf, ptr);
  }

  auto cnt = it->second.count;
  entries.erase(it);
  return cnt;
}

void DicEntryData::addData(EntryTableBuilder* bldr,
                           const util::CsvReader& csv) {
  data.emplace_back();
  auto& v = data.back();
  v.resize(bldr->content_.size());
  for (auto& c : bldr->content_) {
    c.apply(&v, csv);
  }
}

i32 DicEntryData::write(EntryTableBuilder* bldr) {
  auto& buf = bldr->entryDataBuffer;
  auto ptr = buf.position();
  for (auto f : features) {
    buf.writeVarint(static_cast<u32>(f));
  }
  u64 resultPtr = ~u64{0};
  if (data.size() == 1) {
    // write a singular node
    for (auto f : data.front()) {
      buf.writeVarint(static_cast<u32>(f));
    }
    resultPtr = ptr << 1;
    return static_cast<i32>(resultPtr);
  } else {
    // write an aliased node
    buf.writeVarint(data.size());
    for (auto& d : data) {
      for (auto f : d) {
        buf.writeVarint(static_cast<u32>(f));
      }
    }
    resultPtr = (ptr << 1) | 0x1;
  }
  if (resultPtr > std::numeric_limits<i32>::max()) {
    throw new std::range_error("dictionary is too large");
  }
  return static_cast<i32>(resultPtr);
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp
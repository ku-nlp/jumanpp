//
// Created by Arseny Tolmachev on 2017/11/02.
//

#include "spec_compiler.h"
#include <util/csv_reader.h>
#include <util/stl_util.h>
#include "core_config.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace spec {

namespace {
class StorageAssigner {
  util::FlatMap<StringPiece, i32> state_;
  i32 numInts_ = 0;
  i32 numStrings_ = 0;

 public:
  Status assign(FieldDescriptor* fd, StringPiece stringName) {
    switch (fd->fieldType) {
      case FieldType::Int:
        return Status::Ok();
      case FieldType::String:
        fd->stringStorage = assignString(stringName);
        return Status::Ok();
      case FieldType::StringList:
      case FieldType::StringKVList:
        fd->stringStorage = assignString(stringName);
        fd->intStorage = numInts_;
        numInts_ += 1;
        return Status::Ok();
      default:
        return JPPS_NOT_IMPLEMENTED
               << fd->name
               << ": could not assing storage for field, unknown type "
               << fd->fieldType;
    }
  }

  i32 assignString(StringPiece stringName) {
    auto newStr = [&]() { return (numStrings_)++; };

    if (stringName.size() == 0) {
      return newStr();
    } else {
      return state_.findOrInsert(stringName, newStr);
    }
  }

  i32 getNumInts() const { return numInts_; }

  i32 getNumStrings() const { return numStrings_; }
};
}  // namespace

Status SpecCompiler::buildFields() {
  auto spec = &result_->dictionary;
  auto& cols = spec->fields;
  StorageAssigner sa;
  auto& flds = bldr_.fields_;
  for (size_t i = 0; i < flds.size(); ++i) {
    auto f = flds[i];
    cols.emplace_back();  // make one with default constructor
    auto* col = &cols.back();
    col->specIndex = (i32)i;
    StringPiece storName;
    f->fill(col, &storName);
    JPP_RETURN_IF_ERROR(sa.assign(col, storName));
    if (f->isTrieIndex()) {
      spec->indexColumn = (i32)i;
    }
  }

  util::FlatMap<i32, i32> sstor2align;
  sstor2align[spec::InvalidInt] = 0;
  for (auto& f : cols) {
    auto align = sstor2align.findOrInsert(f.stringStorage,
                                          [&f]() { return f.alignment; });
    f.alignment = align;
  }

  spec->numIntStorage = sa.getNumInts();
  spec->numStringStorage = sa.getNumStrings();
  JPP_DCHECK_NE(spec->indexColumn, InvalidInt);
  JPP_DCHECK_NE(spec->numIntStorage, InvalidInt);
  JPP_DCHECK_NE(spec->numStringStorage, InvalidInt);
  return Status::Ok();
}

Status SpecCompiler::buildFeatures() {
  gatherFeatureInfo();
  groupPatternFeatures();
  fillNgramFeatures();
  JPP_RETURN_IF_ERROR(makeDicFeatures());
  JPP_RETURN_IF_ERROR(fillRemainingFeatures());

  return Status::Ok();
}

inline int scoreForUsage(PatternFeatureInfo* i1) {
  switch (i1->usage) {
    case 0b101:  // uni + tri
      return 1;
    case 0b100:  // only tri
      return 10;
    case 0b110:  // uni + bi + tri or bi + tri
    case 0b111:
      return 49;
    case 0b010:  // only bi
      return 50;
    case 0b011:  // uni + bi
      return 51;
    case 0b001:  // only uni
      return 100;
    default:
      return 1000;
  }
}

// we sort pattern features so they will be in
// following order, by usage in ngram features
// 1,3
// 3
// 1,2,3 or 2,3
// 2
// 2,1
// 1
// there could be a large number of unigram-only pattern features
// and they could take L1-cache needed for other stuff
void SpecCompiler::groupPatternFeatures() {
  std::vector<PatternFeatureInfo*> v;
  for (auto& x : patternByCombo_) {
    v.push_back(&x.second);
  }
  std::stable_sort(v.begin(), v.end(),
                   [](PatternFeatureInfo* i1, PatternFeatureInfo* i2) {
                     auto s1 = (scoreForUsage(i1) << 16) + i1->number;
                     auto s2 = (scoreForUsage(i2) << 16) + i2->number;
                     return s1 < s2;
                   });

  i32 idx = 0;
  for (auto p : v) {
    p->number = idx;
    ++idx;
  }
}

AccessFeatureInfo& SpecCompiler::registerFeature(StringPiece name) {
  i32 id = static_cast<i32>(computeByName_.size());
  auto& compF = computeByName_[name];
  if (compF.number == -1) {
    compF.number = id;
  }
  return compF;
}

void SpecCompiler::gatherFeatureInfo() {
  auto& ngrams = bldr_.ngrams_;

  for (auto& ngram : ngrams) {
    auto order = ngram->data.size();

    for (auto& pos : ngram->data) {
      impl::DicEntryFeatures f;
      for (auto& access : pos) {
        auto& compFeature = registerFeature(access.name());
        f.features.push_back(compFeature.number);
      }
      i32 curPatSize = static_cast<i32>(patternByCombo_.size());
      auto& info = patternByCombo_[f];
      if (info.number == -1) {
        info.number = curPatSize;
      }
      info.usage |= (1 << (order - 1));
    }
  }

  for (auto& unk : bldr_.unks_) {
    for (auto& f : unk->sideFeatures_) {
      registerFeature(f.ref.name());
    }
    for (auto& replace : unk->output_) {
      registerFeature(replace.fieldName);
    }
  }

  for (auto& f : bldr_.features_) {
    for (auto& ref : f->trueTransforms_) {
      registerFeature(ref.fieldName);
    }

    for (auto& ref : f->falseTransforms_) {
      registerFeature(ref.fieldName);
    }

    for (auto& ref : f->fields_) {
      auto fld = fieldByName(ref);
      if (util::contains({FieldType::String, FieldType::Int}, fld->fieldType)) {
        registerFeature(ref);
      }
    }
  }

  // computeByName_ will contain all referenced features

  for (auto& f : bldr_.features_) {
    if (!computeByName_.exists(f->name())) {
      // if the feature is not referenced, then ignore it for good
      f->handled = true;
      f->notUsed = true;
      continue;
    }
  }
}

void SpecCompiler::fillNgramFeatures() {
  auto& ngrams = bldr_.ngrams_;
  auto& res = result_->features.ngram;

  i32 number = 0;
  for (auto& ngram : ngrams) {
    res.emplace_back();
    auto& ngramRes = res.back();
    ngramRes.index = number;
    ++number;
    for (auto& pos : ngram->data) {
      impl::DicEntryFeatures f;
      for (auto& access : pos) {
        auto& compFeature = computeByName_[access.name()];
        JPP_DCHECK_NE(compFeature.number, -1);
        f.features.push_back(compFeature.number);
      }
      auto& info = patternByCombo_[f];
      JPP_DCHECK_NE(info.number, -1);
      ngramRes.references.push_back(info.number);
    }
  }
}

Status SpecCompiler::makeDicFeatures() {
  for (auto& col : result_->dictionary.fields) {
    auto it = computeByName_.find(col.name);
    if (it == computeByName_.end()) {
      addDicImport(col, DicImportKind::ImportAsData);
    } else {
      addDicImport(col, DicImportKind::ImportAsFeature);
    }
  }
  for (auto f : bldr_.features_) {
    if (f->handled) {
      continue;
    }

    switch (f->type_) {
      case dsl::FeatureType::MatchValue: {
        JPP_RIE_MSG(makeMatchValueFeature(f), "feature: " << f->name());
        f->handled = true;
        break;
      }
      case dsl::FeatureType::MatchCsv: {
        JPP_RIE_MSG(makeMatchCsvFeature(f), "feature: " << f->name());
        f->handled = true;
        break;
      }
      default:;  // ignore
    }
  }
  auto& dicf = result_->features.dictionary;

  // move features before data
  std::stable_sort(
      dicf.begin(), dicf.end(),
      [](const DicImportDescriptor& d1, const DicImportDescriptor& d2) {
        int k1 = static_cast<int>(d1.kind);
        int k2 = static_cast<int>(d2.kind);
        return k1 < k2;
      });

  result_->features.numDicData = 0;

  // assign dic numbers
  i32 idx = 0;
  for (auto& f : dicf) {
    f.index = idx;
    ++idx;
    if (f.kind == DicImportKind::ImportAsData) {
      auto numDic = result_->features.numDicData;
      f.target = numDic;
      auto fld = fieldByName(f.name);
      fld->dicIndex = ~numDic;
      result_->features.numDicData += 1;
    }
  }

  result_->features.numDicFeatures = 0;
  auto dicIdx = 0;
  for (auto& f : dicf) {
    if (f.kind != DicImportKind::ImportAsData) {
      JPP_DCHECK_IN(f.target, 0, result_->dictionary.fields.size());
      auto& fld = result_->dictionary.fields[f.target];
      if (fld.dicIndex == InvalidInt) {
        fld.dicIndex = dicIdx;
        ++dicIdx;
        result_->features.numDicFeatures += 1;
      }
      auto it = computeByName_.find(f.name);
      if (it != computeByName_.end()) {
        it->second.dicFinalNumber = fld.dicIndex;
      }
      f.target = fld.dicIndex;
    }
  }

  return Status::Ok();
}

i32 SpecCompiler::addDicImport(const FieldDescriptor& descriptor,
                               DicImportKind kind) {
  auto& fdata = result_->features.dictionary;
  i32 idx = static_cast<i32>(fdata.size());
  fdata.emplace_back();
  auto& v = fdata.back();
  v.name = descriptor.name;
  v.index = idx;
  v.kind = kind;
  // v.target will be rewritten by dicIndex later, after it is be decided
  v.target = descriptor.specIndex;
  v.references.push_back(descriptor.specIndex);
  return idx;
}

DicImportDescriptor& SpecCompiler::complexImportCommonInit(
    const dsl::FeatureBuilder* f) {
  auto intSlot = makeIntSlot(1);
  auto& fdata = result_->features.dictionary;
  i32 idx = static_cast<i32>(fdata.size());
  fdata.emplace_back();
  auto& v = fdata.back();
  f->name().assignTo(v.name);
  v.index = idx;
  v.target = intSlot.first;
  v.shift = intSlot.second;
  i32 featureIdx = static_cast<i32>(computeByName_.size());
  auto& cache = computeByName_[f->name()];
  if (cache.number == -1) {
    cache.number = featureIdx;
  }
  cache.shift = v.shift;
  for (auto& x : f->trueTransforms_) {
    auto& cf = computeByName_[x.fieldName];
    JPP_DCHECK_NE(cf.number, -1);
    cache.trueBranch.push_back(cf.number);
  }
  for (auto& x : f->falseTransforms_) {
    auto& cf = computeByName_[x.fieldName];
    JPP_DCHECK_NE(cf.number, -1);
    cache.falseBranch.push_back(cf.number);
  }

  util::CsvReader csv{'\0', '\1'};
  csv.initFromMemory(f->matchData_);
  while (csv.nextLine()) {
    v.data.push_back(csv.field(0).str());
  }

  return v;
}

Status SpecCompiler::makeMatchValueFeature(dsl::FeatureBuilder* f) {
  auto& v = complexImportCommonInit(f);

  if (f->fields_.size() != 1) {
    return JPPS_INVALID_PARAMETER << "# of referenced fields was not 1";
  }

  auto& fldName = f->fields_.front();
  auto fld = fieldByName(fldName);
  JPP_DCHECK_NE(fld->specIndex, InvalidInt);
  v.references.push_back(fld->specIndex);
  using ct = spec::FieldType;

  if (util::contains({ct::StringList, ct::StringKVList}, fld->fieldType)) {
    v.kind = DicImportKind::MatchListKey;
  } else if (fld->fieldType == ct::String) {
    v.kind = DicImportKind::MatchFields;
  } else {
    return JPPS_INVALID_PARAMETER << "unsupported column type: " << fld->name
                                  << " type: " << fld->fieldType;
  }

  return Status::Ok();
}

Status SpecCompiler::makeMatchCsvFeature(dsl::FeatureBuilder* f) {
  auto& v = complexImportCommonInit(f);
  for (auto fldName : f->fields_) {
    auto fld = fieldByName(fldName);
    JPP_DCHECK_NE(fld->specIndex, InvalidInt);
    if (fld->fieldType != spec::FieldType::String) {
      return JPPS_INVALID_PARAMETER << "field " << fld->name
                                    << " was not a String, but "
                                    << fld->fieldType;
    }
    v.references.push_back(fld->specIndex);
  }
  v.kind = DicImportKind::MatchFields;
  return Status::Ok();
}

Status SpecCompiler::build() {
  JPP_RETURN_IF_ERROR(buildFields());
  JPP_RETURN_IF_ERROR(buildFeatures());
  JPP_RETURN_IF_ERROR(buildUnkProcessors());
  JPP_RETURN_IF_ERROR(createTrainSpec());
  buildAliasingSet();

  if (result_->dictionary.fields.size() > JPP_MAX_DIC_FIELDS) {
    return JPPS_INVALID_PARAMETER
           << "current number of fields is greater than JPP_MAX_DIC_FIELDS: "
           << result_->dictionary.fields.size() << " vs " << JPP_MAX_DIC_FIELDS
           << ", please increase it in the build system";
  }
  return Status::Ok();
}

std::pair<i32, i32> SpecCompiler::makeIntSlot(int width) {
  auto newOffset = currentOffset_ + width;
  JPP_DCHECK_LT(width, 32);
  if (newOffset >= 32) {
    auto& cols = result_->dictionary.fields;
    i32 idx = static_cast<i32>(cols.size());
    cols.emplace_back();
    auto* newCol = &cols.back();
    newCol->specIndex = idx;
    newCol->position = 0;
    newCol->name = "__autogen_int_flags_";
    newCol->name += std::to_string(idx);
    newCol->fieldType = FieldType::Int;
    currentFlagField_ = idx;
    currentOffset_ = 0;
  }

  auto offset = currentOffset_;
  currentOffset_ += width;

  return {currentFlagField_, offset};
}

FieldDescriptor* SpecCompiler::fieldByName(StringPiece name, bool allowNull) {
  auto& flds = result_->dictionary.fields;
  for (auto& f : flds) {
    if (name == f.name) {
      return &f;
    }
  }
  JPP_CAPTURE(name);
  JPP_DCHECK(allowNull);
  return nullptr;
}

Status SpecCompiler::fillRemainingFeatures() {
  auto& resFeatures = result_->features;
  auto& compute = resFeatures.computation;
  auto& primitive = resFeatures.primitive;
  resFeatures.totalPrimitives = static_cast<i32>(computeByName_.size());
  compute.resize(computeByName_.size());
  primitive.resize(computeByName_.size());
  for (auto& cf : computeByName_) {
    JPP_CAPTURE(cf.first);
    JPP_DCHECK_IN(cf.second.number, 0, computeByName_.size());
    auto& primData = primitive[cf.second.number];
    primData.index = cf.second.number;
    primData.name = cf.first.str();

    auto feature = findFeature(cf.first);
    if (feature != nullptr) {  // this is
      JPP_RETURN_IF_ERROR(copyPrimitiveInfo(&primData, *feature, cf.second));
      feature->handled = true;
    } else {
      auto fld = fieldByName(cf.first);
      if (!util::contains({FieldType::String, FieldType::Int},
                          fld->fieldType)) {
        return JPPS_INVALID_PARAMETER
               << "only String and Int are allowed as direct features, ["
               << cf.first << "] was used in ngram feature directly";
      }
      primData.references.push_back(cf.second.dicFinalNumber);
      primData.kind = PrimitiveFeatureKind::Copy;
    }

    auto& compData = compute[cf.second.number];
    compData.name = primData.name;
    compData.index = primData.index;
    compData.primitiveFeature = primData.index;
    compData.trueBranch = cf.second.trueBranch;
    compData.falseBranch = cf.second.falseBranch;
  }

  auto& pats = resFeatures.pattern;
  pats.resize(patternByCombo_.size());

  i32 onlyUni = 0;

  for (auto& pf : patternByCombo_) {
    auto pfId = pf.second.number;
    JPP_DCHECK_IN(pfId, 0, pats.size());
    auto& pat = pats[pfId];
    pat.index = pfId;
    pat.usage = pf.second.usage;
    util::copy_insert(pf.first.features, pat.references);
    if (pat.usage == 0x1) {
      onlyUni += 1;
    }
  }

  resFeatures.numUniOnlyPats = onlyUni;

  return Status::Ok();
}

dsl::FeatureBuilder* SpecCompiler::findFeature(StringPiece name) {
  for (auto& f : bldr_.features_) {
    if (name == f->name_) {
      return f;
    }
  }
  return nullptr;
}

Status SpecCompiler::copyPrimitiveInfo(PrimitiveFeatureDescriptor* desc,
                                       const dsl::FeatureBuilder& fBldr,
                                       const AccessFeatureInfo& afi) {
  switch (fBldr.type_) {
    case dsl::FeatureType::MatchValue:
    case dsl::FeatureType::MatchCsv: {
      desc->kind = PrimitiveFeatureKind::SingleBit;
      desc->references.push_back(afi.dicFinalNumber);
      desc->references.push_back(afi.shift);
      break;
    }
    case dsl::FeatureType::Placeholder: {
      desc->kind = PrimitiveFeatureKind::Provided;
      desc->references.push_back(result_->features.numPlaceholders);
      ++result_->features.numPlaceholders;
      break;
    }
    case dsl::FeatureType::ByteLength: {
      desc->kind = PrimitiveFeatureKind::ByteLength;
      JPP_DCHECK_EQ(fBldr.fields_.size(), 1);
      auto fld = fieldByName(fBldr.fields_[0]);
      desc->references.push_back(fld->dicIndex);
      break;
    }
    case dsl::FeatureType::CodepointSize: {
      JPP_DCHECK_EQ(fBldr.fields_.size(), 1);
      auto fld = fieldByName(fBldr.fields_[0]);
      if (fld->isTrieKey) {
        desc->kind = PrimitiveFeatureKind::SurfaceCodepointSize;
      } else {
        desc->kind = PrimitiveFeatureKind::CodepointSize;
        desc->references.push_back(fld->dicIndex);
      }
      break;
    }
    case dsl::FeatureType::Codepoint: {
      desc->kind = PrimitiveFeatureKind::Codepoint;
      JPP_DCHECK_NE(fBldr.intParam_, spec::InvalidInt);
      desc->references.push_back(fBldr.intParam_);
      break;
    }
    case dsl::FeatureType::CodepointType: {
      desc->kind = PrimitiveFeatureKind::CodepointType;
      JPP_DCHECK_NE(fBldr.intParam_, spec::InvalidInt);
      desc->references.push_back(fBldr.intParam_);
      break;
    }
    case dsl::FeatureType::Initial:
    case dsl::FeatureType::Invalid: {
      return JPPS_INVALID_PARAMETER << "invalid feature type";
    }
  }
  return Status::Ok();
}

Status SpecCompiler::createTrainSpec() {
  auto& spec = result_->training;
  spec.surfaceIdx = -1;

  if (!bldr_.train_) {
    return Status::Ok();
  }

  auto& tr = *bldr_.train_;
  for (int i = 0; i < tr.fields.size(); ++i) {
    auto& f = tr.fields[i];
    auto it = computeByName_.find(f.first.name());
    if (f.second != 0 && it == computeByName_.end()) {
      return JPPS_INVALID_PARAMETER
             << "a feature " << f.first.name()
             << " was specified for training, but no ngram feature uses it";
    }
    auto fld = fieldByName(f.first.name());
    spec.fields.emplace_back();
    auto& o = spec.fields.back();
    o.number = i;
    o.fieldIdx = fld->specIndex;
    o.dicIdx = fld->dicIndex;
    o.weight = f.second;
    if (fld->isTrieKey) {
      spec.surfaceIdx = i;
    }
  }

  if (spec.surfaceIdx == -1) {
    return JPPS_INVALID_PARAMETER
           << "trie-indexed field was not selected for training";
  }
  return Status::Ok();
}

Status SpecCompiler::buildUnkProcessors() {
  auto spec = result_;
  i32 cnt = 0;
  for (auto u : bldr_.unks_) {
    spec->unkCreators.emplace_back();
    auto& mkr = spec->unkCreators.back();
    mkr.name = u->name().str();
    mkr.index = cnt++;
    mkr.priority = u->priority_;
    mkr.patternRow = u->pattern_;
    mkr.type = u->type_;
    mkr.charClass = u->charClass_;

    for (auto& x : u->sideFeatures_) {
      auto fname = x.ref.name();
      bool found = false;
      for (auto& f : spec->features.primitive) {
        if (fname == f.name) {
          if (f.kind != PrimitiveFeatureKind::Provided) {
            return JPPS_INVALID_PARAMETER
                   << "unk: " << u->name() << ": "
                   << "can't write feature to non-placeholder " << f.name;
          }
          found = true;
          JPP_DCHECK_EQ(f.references.size(), 1);
          mkr.features.push_back({f.references[0], f.index, x.type});
          break;
        }
      }

      if (!found) {
        return JPPS_INVALID_PARAMETER
               << "failed to find a placeholder with a name: " << fname;
      }
    }

    for (auto& x : u->output_) {
      auto fld = fieldByName(x.fieldName);
      if (fld->fieldType != FieldType::String) {
        return JPPS_INVALID_PARAMETER
               << "unk: " << u->name() << " "
               << " replacing fields other than strings is not supported";
      }
      mkr.replaceFields.push_back(fld->dicIndex);
    }
  }
  return Status::Ok();
}

void SpecCompiler::buildAliasingSet() {
  auto& aset = result_->dictionary.aliasingSet;
  if (result_->training.fields.empty()) {
    // build from features
    bool addedSurface = false;
    for (auto& f : result_->features.dictionary) {
      if (f.kind == DicImportKind::ImportAsFeature) {
        auto fld = fieldByName(f.name);
        aset.push_back(fld->specIndex);
        if (fld->isTrieKey) {
          addedSurface = true;
        }
      }
    }
    if (!addedSurface) {
      for (auto& fld : result_->dictionary.fields) {
        if (fld.isTrieKey) {
          aset.push_back(fld.specIndex);
          break;
        }
      }
    }
  } else {
    for (auto& f : result_->training.fields) {
      if (f.weight != 0) {
        aset.push_back(f.fieldIdx);
      }
    }
  }
  std::sort(aset.begin(), aset.end());
}

}  // namespace spec
}  // namespace core
}  // namespace jumanpp
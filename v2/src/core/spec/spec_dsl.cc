//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "spec_dsl.h"
#include "util/csv_reader.h"
#include "util/flatmap.h"
#include "util/flatset.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace spec {
namespace dsl {

Status ModelSpecBuilder::validateFields() const {
  util::FlatSet<i32> fieldIdx;
  i32 indexCount = 0;
  for (auto fp : fields_) {
    auto& f = *fp;
    JPP_RETURN_IF_ERROR(f.validate());
    if (fieldIdx.count(f.getCsvColumn()) == 0) {
      fieldIdx.insert(f.getCsvColumn());
    } else {
      return Status::InvalidParameter()
             << "coulmn indexes should be unique FAIL: " << f.name();
    }
    if (f.isTrieIndex()) {
      indexCount += 1;
    }
  }
  if (indexCount != 1) {
    return Status::InvalidParameter()
           << "there should exist exactly one indexed field";
  }
  return Status::Ok();
}

Status ModelSpecBuilder::validate() const {
  JPP_RETURN_IF_ERROR(validateFields());
  JPP_RETURN_IF_ERROR(validateNames());
  JPP_RETURN_IF_ERROR(validateFeatures());
  JPP_RETURN_IF_ERROR(validateUnks());
  return Status::Ok();
}

Status ModelSpecBuilder::validateNames() const {
  util::FlatSet<StringPiece> current;
  for (auto fld : fields_) {
    if (current.count(fld->name()) != 0) {
      return Status::InvalidParameter() << "field " << fld->name()
                                        << " has non-unique name";
    }
    current.insert(fld->name());
  }

  for (auto feature : features_) {
    if (current.count(feature->name()) != 0) {
      return Status::InvalidParameter() << "feature " << feature->name()
                                        << " has non-unique name";
    }
  }

  return Status::Ok();
}

Status ModelSpecBuilder::build(AnalysisSpec* spec) const {
  JPP_RETURN_IF_ERROR(validate());
  JPP_RETURN_IF_ERROR(makeFields(spec));
  JPP_RETURN_IF_ERROR(makeFeatures(spec));
  JPP_RETURN_IF_ERROR(createUnkProcessors(spec));
  return Status::Ok();
}

class StorageAssigner {
  util::FlatMap<StringPiece, i32> state_;
  i32 numInts_ = 0;
  i32 numStrings_ = 0;

 public:
  Status assign(FieldDescriptor* fd, StringPiece stringName) {
    switch (fd->columnType) {
      case ColumnType::Int:
        return Status::Ok();
      case ColumnType::String:
        fd->stringStorage = assignString(stringName);
        return Status::Ok();
      case ColumnType::StringList:
        fd->stringStorage = assignString(stringName);
        fd->intStorage = numInts_;
        numInts_ += 1;
        return Status::Ok();
      default:
        return Status::NotImplemented()
               << fd->name
               << ": could not assing storage for field, unknown type "
               << fd->columnType;
    }
  }

  i32 assignString(StringPiece stringName) {
    auto newStr = [&]() { return (numStrings_)++; };

    if (stringName.size() == 0) {
      return newStr();
    } else {
      return state_.getOr(stringName, newStr);
    }
  }

  i32 getNumInts() const { return numInts_; }

  i32 getNumStrings() const { return numStrings_; }
};

Status ModelSpecBuilder::makeFields(AnalysisSpec* anaSpec) const {
  auto spec = &anaSpec->dictionary;
  auto& cols = spec->columns;
  StorageAssigner sa;
  for (size_t i = 0; i < fields_.size(); ++i) {
    auto& f = fields_[i];
    cols.emplace_back();  // make one with default constructor
    auto* col = &cols.back();
    col->index = (i32)i;
    JPP_RETURN_IF_ERROR(f->fill(col, &sa));
    if (f->isTrieIndex()) {
      spec->indexColumn = (i32)i;
    }
  }
  spec->numIntStorage = sa.getNumInts();
  spec->numStringStorage = sa.getNumStrings();
  return Status::Ok();
}

Status ModelSpecBuilder::validateFeatures() const {
  for (auto f : features_) {
    f->handled = false;
    JPP_RETURN_IF_ERROR(f->validate());
  }

  return Status::Ok();
}

Status ModelSpecBuilder::makeFeatures(AnalysisSpec* spec) const {
  auto dicSpec = &spec->dictionary;
  currentFeature_ = 0;
  FeaturesSpec& feats = spec->features;
  util::FlatSet<StringPiece> names;
  collectUsedNames(&names);
  createCopyFeatures(dicSpec->columns, names, &feats.primitive);
  JPP_RETURN_IF_ERROR(
      createRemainingPrimitiveFeatures(dicSpec->columns, &feats.primitive));
  JPP_RETURN_IF_ERROR(createComputeFeatures(&feats));
  JPP_RETURN_IF_ERROR(checkNoFeatureIsLeft());
  JPP_RETURN_IF_ERROR(createPatternsAndFinalFeatures(&feats));
  feats.totalPrimitives = currentFeature_;

  return Status::Ok();
}

void ModelSpecBuilder::collectUsedNames(
    util::FlatSet<StringPiece>* names) const {
  for (auto c : combinators_) {
    for (auto& vec : c->data) {
      for (auto& ref : vec) {
        names->insert(ref.name());
      }
    }
  }
  for (auto f : features_) {
    for (auto& fld : f->fields_) {
      names->insert(fld);
    }
    for (auto& x : f->trueTransforms_) {
      names->insert(x.fieldName);
    }
    for (auto& x : f->falseTransforms_) {
      names->insert(x.fieldName);
    }
  }
}

void ModelSpecBuilder::createCopyFeatures(
    const util::ArraySlice<FieldDescriptor>& fields,
    const util::FlatSet<StringPiece>& names,
    std::vector<PrimitiveFeatureDescriptor>* result) const {
  auto field_types = {ColumnType::String, ColumnType::Int};

  for (auto& f : fields) {
    if (!util::contains(field_types, f.columnType)) {
      continue;
    }

    if (names.count(f.name) != 0) {
      PrimitiveFeatureDescriptor copy;
      copy.name = f.name;
      copy.index = currentFeature_++;
      copy.kind = PrimitiveFeatureKind::Copy;
      copy.references.push_back(f.index);
      result->push_back(copy);
    }
  }
}

Status findOnlyFieldForFeature(util::ArraySlice<FieldDescriptor> fields,
                               StringPiece featureName,
                               util::ArraySlice<StringPiece> references,
                               i32* result) {
  if (references.size() != 1) {
    return Status::InvalidParameter()
           << featureName << ": feature can have only one parameter";
  }
  auto fname = references[0];
  auto res = std::find_if(
      fields.begin(), fields.end(),
      [fname](const FieldDescriptor& fd) -> bool { return fname == fd.name; });

  if (res == fields.end()) {
    return Status::InvalidState() << "feature " << featureName
                                  << " was referring to unknown field "
                                  << fname;
  }

  *result = res->index;

  return Status::Ok();
}

Status ModelSpecBuilder::createRemainingPrimitiveFeatures(
    const util::ArraySlice<FieldDescriptor>& fields,
    std::vector<PrimitiveFeatureDescriptor>* result) const {
  for (auto f : features_) {
    auto type = f->type_;
    PrimitiveFeatureDescriptor pfd;
    pfd.kind = PrimitiveFeatureKind::Invalid;

    if (type == FeatureType::Placeholder) {
      f->handled = true;
      pfd.kind = PrimitiveFeatureKind::Provided;
    } else if (type == FeatureType::Length) {
      i32 index = 0;
      JPP_RETURN_IF_ERROR(
          findOnlyFieldForFeature(fields, f->name(), f->fields_, &index));
      f->handled = true;
      pfd.kind = PrimitiveFeatureKind::Length;
      pfd.references.push_back(index);
    } else if (type == FeatureType::MatchValue) {
      if (f->fields_.size() == 1) {
        i32 index = 0;
        JPP_RETURN_IF_ERROR(
            findOnlyFieldForFeature(fields, f->name(), f->fields_, &index));
        auto res = fields[index];
        f->handled = true;
        pfd.references.push_back(res.index);
        pfd.matchData.push_back(f->matchData_.str());
        using ct = ColumnType;
        if (util::contains({ct::String, ct::Int}, res.columnType)) {
          pfd.kind = PrimitiveFeatureKind::MatchDic;
        } else if (ColumnType::StringList == res.columnType) {
          pfd.kind = PrimitiveFeatureKind::MatchAnyDic;
        } else {
          return Status::NotImplemented()
                 << "building match feature: unupported column class "
                 << res.columnType << " for feature " << f->name();
        }
      }
    }

    if (f->handled) {
      pfd.name = f->name().str();
      pfd.index = currentFeature_++;
      result->push_back(pfd);
    }
  }
  return Status::Ok();
}

Status checkTfType(const FieldExpressionBldr& ex) {
  if (ex.type != TransformType::Value) {
    return Status::InvalidParameter()
           << "match fields support only value references";
  }
  return Status::Ok();
}

Status readMatchDataCsv(StringPiece csvdata, size_t columns,
                        std::vector<std::string>* result) {
  util::CsvReader csvr;
  JPP_RETURN_IF_ERROR(csvr.initFromMemory(csvdata));
  while (csvr.nextLine()) {
    if (csvr.numFields() != columns) {
      return Status::InvalidParameter()
             << "on line " << csvr.lineNumber() << " of csv there were "
             << csvr.numFields() << " columns, expected " << columns;
    }
    for (i32 i = 0; i < columns; ++i) {
      result->push_back(csvr.field(i).str());
    }
  }
  return Status::Ok();
}

Status ModelSpecBuilder::createComputeFeatures(FeaturesSpec* fspec) const {
  util::FlatMap<StringPiece, i32> name2prim;
  name2prim.reserve(fspec->primitive.size() * 2);
  for (auto& f : fspec->primitive) {
    name2prim[f.name] = f.index;
  }

  for (auto f : features_) {
    if (f->handled) {
      continue;
    }

    if (f->trueTransforms_.size() == 0 && f->falseTransforms_.size() == 0) {
      continue;
    }

    auto ftype = f->type_;
    ComputationFeatureDescriptor cfd{};

    for (auto& tex : f->trueTransforms_) {
      JPP_RETURN_IF_ERROR(checkTfType(tex));
      auto idx = name2prim.find(tex.fieldName);
      if (idx == name2prim.end()) {
        return Status::InvalidState() << f->name()
                                      << ": could not find primitive feature "
                                      << tex.fieldName;
      }
      cfd.trueBranch.push_back(idx->second);
    }

    for (auto& tex : f->falseTransforms_) {
      JPP_RETURN_IF_ERROR(checkTfType(tex));
      auto idx = name2prim.find(tex.fieldName);
      if (idx == name2prim.end()) {
        return Status::InvalidState() << f->name()
                                      << ": could not find primitive feature "
                                      << tex.fieldName;
      }
      cfd.falseBranch.push_back(idx->second);
    }

    for (auto& ref : f->fields_) {
      auto idx = name2prim.find(ref);
      if (idx == name2prim.end()) {
        return Status::InvalidState()
               << f->name() << ": could not find primitive feature " << ref;
      }
      MatchReference mr;
      mr.featureIdx = idx->second;
      auto& prim = fspec->primitive[idx->second];
      if (prim.kind == PrimitiveFeatureKind::Copy) {
        mr.dicFieldIdx = prim.references[0];
      } else {
        mr.dicFieldIdx = -1;
      }
      cfd.matchReference.push_back(mr);
    }

    if (f->matchData_.size() == 0) {
      return Status::InvalidParameter() << f->name()
                                        << " match feature needs match data";
    }

    if (ftype == FeatureType::MatchValue) {
      cfd.matchData.push_back(f->matchData_.str());
      f->handled = true;
    } else if (ftype == FeatureType::MatchCsv) {
      JPP_RETURN_IF_ERROR(readMatchDataCsv(
          f->matchData_, cfd.matchReference.size(), &cfd.matchData));
      f->handled = true;
    }

    if (f->handled) {
      cfd.index = currentFeature_++;
      cfd.name = f->name().str();
      fspec->computation.push_back(cfd);
    }
  }
  return Status::Ok();
}

Status ModelSpecBuilder::checkNoFeatureIsLeft() const {
  for (auto f : features_) {
    if (!f->handled) {
      return Status::InvalidState() << f->name() << ": feature was not handled";
    }
  }
  return Status::Ok();
}

Status convertRefToId(const util::FlatMap<StringPiece, i32>& f2id,
                      const FeatureRef& ref, std::vector<i32>* result) {
  auto res = f2id.find(ref.name());
  if (res == f2id.end()) {
    return Status::InvalidState() << "feature " << ref.name()
                                  << " was not in set of primitive features";
  }
  result->push_back(res->second);
  return Status::Ok();
}

struct Vectori32Hash {
  std::hash<i32> impl;
  size_t operator()(const std::vector<i32>& vec) const {
    size_t result = 0xdeadbeef123edafULL;
    for (int i = 0; i < vec.size(); ++i) {
      result ^= 31 * impl(vec[i]) ^ impl(i);
    }
    return result ^ impl((int)vec.size());
  }
};

Status ModelSpecBuilder::createPatternsAndFinalFeatures(
    FeaturesSpec* spec) const {
  util::FlatMap<StringPiece, i32> feature2id;

  // feature names are checked to be unique
  for (auto& f : spec->primitive) {
    feature2id[f.name] = f.index;
  }
  for (auto& f : spec->computation) {
    feature2id[f.name] = f.index;
  }

  util::FlatMap<std::vector<i32>, i32, Vectori32Hash> pattern2id;
  std::vector<i32> buffer;
  i32 finalIdx = 0;
  for (auto comb : combinators_) {
    FinalFeatureDescriptor ffd;
    ffd.index = finalIdx++;
    for (auto& patStrings : comb->data) {
      buffer.clear();
      for (auto& pat : patStrings) {
        JPP_RETURN_IF_ERROR(convertRefToId(feature2id, pat, &buffer));
      }
      auto res = pattern2id.find(buffer);
      i32 nextId = static_cast<i32>(pattern2id.size());
      if (res == pattern2id.end()) {
        pattern2id[buffer] = nextId;
      } else {
        nextId = res->second;
      }
      ffd.references.push_back(nextId);
    }
    spec->final.push_back(ffd);
  }

  for (auto& p : pattern2id) {
    PatternFeatureDescriptor pat;
    pat.index = p.second;
    pat.references = p.first;
    spec->pattern.push_back(pat);
  }

  return Status::Ok();
}

Status ModelSpecBuilder::validateUnks() const {
  for (auto unk : unks_) {
    JPP_RETURN_IF_ERROR(unk->validate());
  }
  return Status::Ok();
}

Status fillFieldExp(StringPiece name, const FieldDescriptor& fld, const FieldExpressionBldr& feb, FieldExpression* res) {
  res->fieldIndex = fld.index;
  switch (feb.type) {
    case TransformType::Value:
      res->kind = FieldExpressionKind::ReplaceWithMatch;
      break;
    case TransformType::ReplaceInt:
      res->kind = FieldExpressionKind::ReplaceInt;
      res->intConstant = feb.transformInt;
      break;
    case TransformType::ReplaceString:
      res->kind = FieldExpressionKind::ReplaceString;
      res->stringConstant = feb.transformString.str();
      break;
    case TransformType::AppendString:
      return Status::InvalidParameter()
             << name << ": field " << fld.name
             << " appending strings is not supported";
    default:
      return Status::NotImplemented() << name << ": field " << fld.name
                                      << " combination was not implemented";
  }
  return Status::Ok();
}

Status ModelSpecBuilder::createUnkProcessors(AnalysisSpec* spec) const {
  util::FlatMap<StringPiece, FieldDescriptor*> fld2id;
  util::FlatMap<StringPiece, PrimitiveFeatureDescriptor*> feat2id;

  for (auto& f : spec->dictionary.columns) {
    fld2id[f.name] = &f;
  }
  for (auto& f : spec->features.primitive) {
    feat2id[f.name] = &f;
  }

  i32 cnt = 0;
  for (auto u : unks_) {
    UnkMaker mkr;
    mkr.name = u->name().str();
    mkr.index = cnt++;
    mkr.patternRow = u->pattern_;
    mkr.type = u->type_;
    mkr.charClass = u->charClass_;

    for (auto& x : u->surfaceFeatures_) {
      UnkMakerFeature fe;
      auto fld = feat2id.at(x.ref.name());
      fe.type = x.type;
      fe.reference = fld->index;
      mkr.features.push_back(fe);
    }

    for (auto& x : u->output_) {
      FieldExpression fe;
      auto fld = fld2id.at(x.fieldName);
      JPP_RETURN_IF_ERROR(fillFieldExp(u->name(), *fld, x, &fe));
      mkr.outputExpressions.push_back(fe);
    }
  }
  return Status::Ok();
}

Status FieldBuilder::validate() const {
  if (name_.size() == 0) {
    return Status::InvalidParameter() << "field name should not be empty";
  }
  if (csvColumn_ <= 0) {
    return Status::InvalidParameter()
           << "column number must be a positive integer, field " << name_
           << " has " << csvColumn_;
  }
  if (columnType_ == ColumnType::Error) {
    return Status::InvalidParameter() << "coumn " << name_
                                      << " do not have type";
  }
  if (trieIndex_ && columnType_ != ColumnType::String) {
    return Status::InvalidParameter()
           << "only string field can be indexed, field " << this->name_
           << " is not one";
  }
  if (emptyValue_.size() > 0 && columnType_ == ColumnType::Int) {
    return Status::InvalidParameter()
           << "empty field does not make sense on int fields like "
           << this->name_;
  }

  if (stringStorage_.size() > 0) {
    if (!util::contains({ColumnType::StringList, ColumnType::String},
                        columnType_)) {
      return Status::InvalidParameter() << "string storage can be specified "
                                           "only for string or stringList "
                                           "typed columns";
    }
  }

  return Status::Ok();
}

Status FieldBuilder::fill(FieldDescriptor* descriptor,
                          StorageAssigner* sa) const {
  descriptor->position = csvColumn_;
  descriptor->columnType = columnType_;
  descriptor->emptyString = emptyValue_.str();
  descriptor->isTrieKey = trieIndex_;
  descriptor->name = name_.str();
  auto storName = stringStorage_.size() == 0 ? name() : stringStorage_;
  JPP_RETURN_IF_ERROR(sa->assign(descriptor, storName));
  return Status::Ok();
}

Status FeatureBuilder::validate() const {
  if (type_ == FeatureType::Initial) {
    return Status::InvalidParameter() << "feature " << name_
                                      << " was not initialized";
  }
  if (type_ == FeatureType::Invalid) {
    return Status::InvalidParameter() << "feature " << name_
                                      << " was initialized more than one time";
  }
  if (type_ == FeatureType::Length && fields_.size() != 1) {
    return Status::InvalidState() << "feature " << name_
                                  << " can contain only one field reference";
  }

  if ((trueTransforms_.size() != 0) ^ (falseTransforms_.size() != 0)) {
    return Status::InvalidParameter()
           << name_
           << ": matching features must ether have both branches or have none";
  }

  if (trueTransforms_.size() != 0 && falseTransforms_.size() != 0) {
    if (!util::contains({FeatureType::MatchCsv, FeatureType::MatchValue},
                        type_)) {
      return Status::InvalidParameter()
             << name_ << ": only matching features can have branches";
    }
  }

  return Status::Ok();
}

Status UnkProcBuilder::validate() const {
  if (name_.size() == 0) {
    return Status::InvalidParameter() << "unk processor must have a name";
  }
  if (type_ == UnkMakerType::Invalid) {
    return Status::InvalidParameter() << name_
                                      << ": unk processor was not initialized";
  }
  if (pattern_ == -1) {
    return Status::InvalidParameter()
           << name_ << ": unk processor must have pattern specified";
  }
  return Status::Ok();
}

}  // dsl
}  // spec
}  // core
}  // jumanpp

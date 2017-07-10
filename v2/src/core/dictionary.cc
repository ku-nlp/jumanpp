//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dictionary.h"
#include "util/flatmap.h"
#include "util/flatset.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace dic {

struct PairHash {
  std::hash<i32> h;
  size_t operator()(const std::pair<i32, i32>& p) const {
    return 31 * h(p.first) + h(p.second);
  }
};

class RuntimeInfoCompiler {
  const spec::AnalysisSpec& spec_;
  const FieldsHolder& fields_;
  const EntriesHolder& entries_;

  std::vector<util::FlatMap<StringPiece, i32>> word2id_;
  util::FlatMap<i32, i32> ptrsOfEntries;

 public:
  RuntimeInfoCompiler(const spec::AnalysisSpec& spec_,
                      const FieldsHolder& fields_,
                      const EntriesHolder& entries_)
      : spec_(spec_), fields_(fields_), entries_(entries_) {}

  Status initialize() {
    util::FlatSet<std::pair<i32, i32>, PairHash> usedStrings;

    for (auto& x : spec_.features.primitive) {
      if (!x.matchData.empty()) {
        for (auto fld : x.references) {
          auto si = strings(fld);
          if (si == -1) {
            return Status::InvalidState()
                   << x.name << ": string storage was not assigned";
          }
          usedStrings.insert({si, fld});
        }
      }
    }

    for (auto& x : spec_.features.computation) {
      if (!x.matchData.empty()) {
        for (auto& ref : x.matchReference) {
          auto fld = ref.dicFieldIdx;
          auto si = strings(fld);
          if (si == -1) {
            return Status::InvalidState()
                   << x.name << ": string storage was not assigned";
          }
          usedStrings.insert({si, fld});
        }
      }
    }

    word2id_.resize(spec_.dictionary.numStringStorage);

    for (auto& storage : usedStrings) {
      JPP_RETURN_IF_ERROR(buildIdx(storage.first, storage.second));
    }

    JPP_RETURN_IF_ERROR(resolveEntryPtrs());

    return Status::Ok();
  }

  Status resolveEntryPtrs() {
    std::vector<i32> patterns;
    for (auto& x : spec_.unkCreators) {  // raw line number is 1-based
      patterns.push_back(x.patternRow - 1);
    }
    if (patterns.empty()) {
      return Status::Ok();
    }

    std::sort(patterns.begin(), patterns.end());
    auto last = std::unique(patterns.begin(), patterns.end());
    patterns.erase(last, patterns.end());

    auto esize = entries_.entrySize;
    auto ents = entries_.entries.raw();
    i32 ptr = 0;
    i32 cnt = 0;
    auto it = patterns.cbegin();
    while (it != patterns.cend()) {
      while (*it != cnt) {
        for (int i = 0; i < esize; ++i) {
          int dummy;
          if (!ents.readOne(&dummy)) {
            return Status::InvalidState() << "int storage finished, were not "
                                             "able to resolve pointers!";
          }
        }
        ptr = ents.pointer();
        cnt += 1;
      }
      ptrsOfEntries[cnt] = ptr;
      ++it;
    }
    return Status::Ok();
  }

  Status buildIdx(i32 storage, i32 field) {
    auto& data = word2id_[storage];
    if (!data.empty()) {
      return Status::Ok();
    }

    auto& fld = fields_.at(field);
    impl::StringStorageTraversal trav{fld.strings.data()};
    StringPiece sp;
    while (trav.next(&sp)) {
      data[sp] = trav.position();
    }

    return Status::Ok();
  }

  i32 strings(i32 fld) const {
    return spec_.dictionary.columns[fld].stringStorage;
  }

  bool isEmpty(StringPiece sp, i32 fldNo) const {
    auto& fld = spec_.dictionary.columns[fldNo];
    return fld.emptyString == sp;
  }

  Status copyOneField(i32 field, const std::vector<std::string>& strs,
                      std::vector<i32>* result) {
    auto& strmap = word2id_[strings(field)];
    for (auto& s : strs) {
      if (isEmpty(s, field)) {
        result->push_back(0);
        continue;
      }

      auto res = strmap.find(s);
      if (res == strmap.end()) {
        return Status::InvalidState()
               << " when loading single column csv match data field #" << field
               << " storage did not contain string " << s;
      }
      result->push_back(res->second);
    }
    return Status::Ok();
  }

  Status copyTable(const std::vector<i32>& fields,
                   const std::vector<std::string>& data,
                   std::vector<i32>* result) {
    for (int i = 0; i < data.size(); ++i) {
      auto fldIdx = fields.at(i % fields.size());
      auto& s = data[i];

      if (isEmpty(s, fldIdx)) {
        result->push_back(0);
        continue;
      }

      auto sidx = strings(fldIdx);
      auto& strmap = word2id_[sidx];
      auto res = strmap.find(s);
      if (res == strmap.end()) {
        return Status::InvalidState()
               << "when loading multicolumn csv match data field #" << fldIdx
               << " storage did not contain string " << s;
      }
      result->push_back(res->second);
    }

    return Status::Ok();
  }

  void sortTable(size_t entry, const std::vector<i32>& data,
                 std::vector<i32>* result) {
    std::vector<util::ArraySlice<i32>> slices;
    size_t nslices = data.size() / entry;
    for (int i = 0; i < nslices; ++i) {
      slices.emplace_back(data, i * entry, entry);
    }

    auto cmp = [entry](const util::ArraySlice<i32>& l,
                       const util::ArraySlice<i32>& r) -> bool {
      for (int i = 0; i < entry; ++i) {
        i32 le = l[i];
        i32 re = r[i];
        if (le == re) continue;
        return le < re;
      }
      return false;
    };

    util::sort(slices, cmp);
    for (auto& sl : slices) {
      util::copy_insert(sl, *result);
    }
  }

  Status compileFeatures(features::FeatureRuntimeInfo* fri) {
    auto& specf = spec_.features;

    for (auto& pfd : specf.primitive) {
      if (pfd.kind == spec::PrimitiveFeatureKind::Provided) {
        fri->placeholderMapping.push_back(pfd.index);
      }
    }

    auto& prim = fri->primitive;
    for (auto& pfd : specf.primitive) {
      prim.emplace_back();
      auto& pf = prim.back();
      pf.name = pfd.name;
      pf.index = pfd.index;
      pf.kind = pfd.kind;
      if (pfd.kind == spec::PrimitiveFeatureKind::Provided) {
        for (int i = 0; i < fri->placeholderMapping.size(); ++i) {
          auto idx = fri->placeholderMapping[i];
          if (idx == pfd.index) {
            pf.references.push_back(i);
          }
        }
        if (pf.references.empty()) {
          return Status::InvalidState() << "placeholder feature " << pfd.name
                                        << " could not be processed";
        }
      } else {
        std::copy(pfd.references.begin(), pfd.references.end(),
                  std::back_inserter(pf.references));
      }
      if (pfd.references.empty()) {
        if (!pfd.matchData.empty()) {
          return Status::InvalidParameter()
                 << pfd.name
                 << ": field had data, but did not have any references";
        }
      }
      if (pfd.references.size() == 1) {
        JPP_RETURN_IF_ERROR(
            copyOneField(pfd.references[0], pfd.matchData, &pf.matchData));
      } else if (pfd.references.size() > 1 &&
                 pfd.matchData.size() % pfd.references.size() == 0) {
        JPP_RETURN_IF_ERROR(
            copyTable(pfd.references, pfd.matchData, &pf.matchData));
      } else if (!pfd.matchData.empty()) {
        return Status::InvalidParameter()
               << pfd.name
               << ": field had data, but could not combine it with dictionary";
      }
    }

    auto& comp = fri->compute;
    std::vector<i32> fields;
    std::vector<i32> temp;
    for (auto& cfd : specf.computation) {
      comp.emplace_back();
      auto& cf = comp.back();
      cf.name = cfd.name;
      cf.index = cfd.index;
      for (auto& x : cfd.matchReference) {
        cf.references.push_back(x.dicFieldIdx);
      }
      util::copy_insert(cfd.trueBranch, cf.trueBranch);
      util::copy_insert(cfd.falseBranch, cf.falseBranch);
      fields.clear();
      for (auto& fr : cfd.matchReference) {
        fields.push_back(fr.dicFieldIdx);
      }
      temp.clear();
      JPP_RETURN_IF_ERROR(copyTable(fields, cfd.matchData, &temp));
      sortTable(fields.size(), temp, &cf.matchData);
    }

    auto& pat = fri->patterns;
    for (auto& pfd : specf.pattern) {
      pat.emplace_back();
      auto& pf = pat.back();
      pf.index = pfd.index;
      util::copy_insert(pfd.references, pf.arguments);
    }

    auto& ngram = fri->ngrams;
    for (auto& ng : specf.final) {
      ngram.emplace_back();
      auto& nf = ngram.back();
      nf.index = ng.index;
      util::copy_insert(ng.references, nf.arguments);
    }

    return Status::Ok();
  }

  Status compileUnks(analysis::UnkMakersInfo* result) {
    auto& makers = result->makers;
    for (auto& unk : spec_.unkCreators) {
      makers.emplace_back();
      auto& mk = makers.back();
      mk.index = unk.index;
      mk.name = unk.name;
      mk.type = unk.type;
      mk.charClass = unk.charClass;
      util::copy_insert(unk.features, mk.features);
      mk.priority = unk.priority;
      mk.patternPtr = ptrsOfEntries.at(unk.patternRow - 1);
      for (auto& x : unk.outputExpressions) {
        mk.output.push_back(x.fieldIndex);
      }
    }

    return Status::Ok();
  }

  Status compile(RuntimeInfo* result) {
    JPP_RETURN_IF_ERROR(compileFeatures(&result->features));
    JPP_RETURN_IF_ERROR(compileUnks(&result->unkMakers));
    result->unkMakers.numPlaceholders =
        static_cast<i32>(result->features.placeholderMapping.size());
    return Status::Ok();
  }
};

Status fillEntriesHolder(const BuiltDictionary& dic, EntriesHolder* result) {
  result->entrySize = static_cast<i32>(dic.fieldData.size());
  result->entries = impl::IntStorageReader{dic.entryData};
  result->entryPtrs = impl::IntStorageReader{dic.entryPointers};
  return result->trie.loadFromMemory(dic.trieContent);
}

Status FieldsHolder::load(const BuiltDictionary& dic) {
  for (i32 index = 0; index < dic.fieldData.size(); ++index) {
    auto& f = dic.fieldData[index];
    DictionaryField df{index,
                       f.name,
                       f.colType,
                       impl::IntStorageReader{f.fieldContent},
                       impl::StringStorageReader{f.stringContent},
                       f.emptyValue,
                       f.stringStorageIdx};
    fields_.push_back(df);
  }
  return Status::Ok();
}

Status DictionaryHolder::load(const BuiltDictionary& dic) {
  JPP_RETURN_IF_ERROR(fields_.load(dic));
  JPP_RETURN_IF_ERROR(fillEntriesHolder(dic, &entries_));
  return Status::Ok();
}

Status DictionaryHolder::compileRuntimeInfo(const spec::AnalysisSpec& spec,
                                            RuntimeInfo* runtime) {
  RuntimeInfoCompiler compiler{spec, fields_, entries_};
  JPP_RETURN_IF_ERROR(compiler.initialize());
  JPP_RETURN_IF_ERROR(compiler.compile(runtime));
  return Status::Ok();
}

}  // namespace dic
}  // namespace core
}  // namespace jumanpp
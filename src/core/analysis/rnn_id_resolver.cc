//
// Created by Arseny Tolmachev on 2017/05/31.
//

#include "rnn_id_resolver.h"
#include <util/fast_hash.h>
#include "core/analysis/score_processor.h"
#include "util/char_buffer.h"
#include "util/csv_reader.h"
#include "util/serialization.h"
#include "util/serialization_flatmap.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace analysis {
namespace rnn {

namespace {
struct RnnIdResolverBuilder {
  dic::DoubleArrayBuilder knownBuilder;
  dic::DoubleArrayBuilder unkBuilder;
  util::CharBuffer<> knownStrings;
  util::CharBuffer<> unkStrings;
  RnnReprBuilder repr;
  std::string eos = "</s>";
  std::string separator;
  std::string unkToken;
  std::vector<util::FlatMap<StringPiece, i32>> fld2pos;
  i32 eosId = -1;
  i32 unkId = -1;
  util::CsvReader csv;

  Status resolveFields(const std::vector<std::string>& names,
                       const dic::DictionaryHolder& dic,
                       std::vector<u32>* fieldNums) {
    for (auto& name : names) {
      auto fld = dic.fieldByName(name);
      if (fld == nullptr) {
        return JPPS_INVALID_PARAMETER
               << "could not find a field with name: " << name
               << " in dictionary";
      }
      if (fld->columnType != spec::FieldType::String) {
        return JPPS_INVALID_PARAMETER
               << "can use only string-typed field in RNN, " << name << " was "
               << fld->columnType;
      }
      fieldNums->push_back(static_cast<u32>(fld->idxInEntry));
      dic::impl::StringStorageTraversal sst{fld->strings};
      fld2pos.emplace_back();
      auto& fldPos = fld2pos.back();
      StringPiece key;
      while (sst.next(&key)) {
        fldPos[key] = sst.position();
      }
    }
    return Status::Ok();
  }

  Status buildRepr(StringPiece word, bool* known) {
    csv.initFromMemory(word);
    if (!csv.nextLine()) {
      return JPPS_INVALID_STATE << "failed to parse line: " << word;
    }
    if (csv.numFields() != fld2pos.size()) {
      return JPPS_INVALID_PARAMETER << "failed to split " << word << " into "
                                    << fld2pos.size() << " components using "
                                    << separator << " as separator";
    }
    for (i32 i = 0; i < fld2pos.size(); ++i) {
      const auto& map = fld2pos[i];
      auto str = csv.field(i);
      auto it = map.find(str);
      if (it == map.end()) {
        *known = false;
        repr.addString(str);
      } else {
        repr.addInt(it->second);
      }
    }
    return Status::Ok();
  }

  Status loadData(util::ArraySlice<StringPiece> rnndic) {
    if (separator.size() != 1) {
      return JPPS_INVALID_STATE
             << "we support RNN separators only of 1 byte length";
    }
    csv = util::CsvReader{separator[0], '\0'};
    for (i32 i = 0; i < rnndic.size(); ++i) {
      auto rnnWord = rnndic[i];
      if (rnnWord == eos) {
        eosId = i;
        continue;
      }

      if (rnnWord == unkToken) {
        unkId = i;
        continue;
      }

      repr.reset();
      bool known = true;
      JPP_RIE_MSG(buildRepr(rnnWord, &known),
                  "word=" << rnnWord << " at line=" << i);
      auto repr = this->repr.repr();
      if (known) {
        knownStrings.import(&repr);
        knownBuilder.add(repr, i);
      } else {
        unkStrings.import(&repr);
        unkBuilder.add(repr, i);
      }
    }

    if (eosId == -1) {
      return JPPS_INVALID_PARAMETER
             << "rnn dic file did not contain BOS/EOS marker (" << eos << ")";
    }

    if (!unkToken.empty() && unkId == -1) {
      return JPPS_INVALID_PARAMETER
             << "rnn dic did not contain UNK word marker (" << unkToken << ")";
    }

    return Status::Ok();
  }

  Status makeIndices() {
    JPP_RETURN_IF_ERROR(knownBuilder.build());
    JPP_RETURN_IF_ERROR(unkBuilder.build());
    return Status::Ok();
  }
};
}  // namespace

Status RnnIdResolver::build(const dic::DictionaryHolder& dic,
                            const RnnInferenceConfig& cfg,
                            util::ArraySlice<StringPiece> rnndic) {
  RnnIdResolverBuilder bldr;
  bldr.separator = cfg.fieldSeparator;
  bldr.unkToken = cfg.unkSymbol;
  bldr.eos = cfg.eosSymbol;
  JPP_RETURN_IF_ERROR(bldr.resolveFields(cfg.rnnFields, dic, &fields_));
  JPP_RETURN_IF_ERROR(bldr.loadData(rnndic));
  JPP_RETURN_IF_ERROR(bldr.makeIndices());
  if (bldr.eosId != 0) {
    return JPPS_NOT_IMPLEMENTED << "we don't support if EOS/BOS token is not 0";
  }
  unkId_ = bldr.unkId;
  knownIndex_.plunder(&bldr.knownBuilder);
  unkIndex_.plunder(&bldr.unkBuilder);
  return Status::Ok();
}

StringPiece RnnIdResolver::reprOf(RnnReprBuilder* bldr, EntryPtr eptr,
                                  util::ArraySlice<i32> features,
                                  const ExtraNodesContext* xtra) const {
  bldr->reset();
  for (auto fld : fields_) {
    auto feature = features.at(fld);
    if (feature >= 0) {
      bldr->addInt(feature);
    } else {
      auto node = xtra->node(eptr);
      bldr->addString(node->header.unk.surface);
    }
  }
  return bldr->repr();
}

Status RnnIdResolver::resolveIdsAtGbeam(RnnIdContainer* ids, Lattice* lat,
                                        const ExtraNodesContext* xtra) const {
  auto numBnds = lat->createdBoundaryCount();
  ids->reset(numBnds, lat->config().globalBeamSize);
  auto last = lat->boundary(numBnds - 1);
  auto bos1 = lat->boundary(1);
  auto& bosBeam = bos1->starts()->beamData().at(0);
  ids->ptrCache_[&bosBeam.ptr] = ids->crdCache_[{1, 0, 0}];

  LatticeNodePtr eos{static_cast<u16>(numBnds - 1), 0};
  auto rawGbeamPtr = lat->lastGbeamRaw();
  auto gbeam = last->ends()->globalBeam();
  util::ArraySlice<BeamCandidate> gbeamCands{
      reinterpret_cast<const BeamCandidate*>(rawGbeamPtr), gbeam.size()};

  for (int topn = 0; topn < gbeam.size(); ++topn) {
    auto& beam = gbeam.at(topn);
    auto& rawBeam = gbeamCands.at(topn);
    auto fakeConnection = ids->fakeConnection(eos, beam, rawBeam);
    ids->addPath(this, lat, fakeConnection, xtra);
  }

  return Status::Ok();
}

Status RnnIdResolver::setState(util::ArraySlice<u32> fields, StringPiece known,
                               StringPiece unknown, i32 unkid) {
  util::copy_insert(fields, fields_);
  unkId_ = unkid;
  JPP_RETURN_IF_ERROR(knownIndex_.loadFromMemory(known));
  JPP_RETURN_IF_ERROR(unkIndex_.loadFromMemory(unknown));
  return Status::Ok();
}

std::pair<RnnNode*, RnnNode*> RnnIdContainer::addPrevChain(
    const RnnIdResolver* resolver, const Lattice* lat,
    const ConnectionPtr* cptr, const ExtraNodesContext* xtra) {
  auto prevPair = ptrCache_.emplace(cptr, nullptr);
  if (prevPair.second) {
    auto span = addPrevChain(resolver, lat, cptr->previous, xtra);  // recursion
    auto prev = span.second;
    auto& nodePtr = prevPair.first->second;
    auto& coord = resolveId(resolver, lat, cptr, xtra);
    auto rnnIdU32 =
        static_cast<u32>(coord.rnnId) | (static_cast<u64>(coord.length) << 32);
    auto hash = util::hashing::FastHash1{prev->hash}.mix(rnnIdU32).result();
    auto it = crdCache_.find(coord);

    if (it != crdCache_.end()) {
      auto* cached = it->second;
      do {
        if (cached->hash == hash) {
          // we can collapse a path in the RNN lattice
          // but we still need to publish the score
          nodePtr = it->second;
          addScore(nodePtr, cptr);
          return std::make_pair(nodePtr, nodePtr);
        }
        cached = cached->nextInBnd;
      } while (cached != nullptr);
    }

    nodePtr = alloc_->allocate<RnnNode>();
    nodePtr->prev = prev;
    nodePtr->hash = hash;
    nodePtr->id = coord.rnnId;
    nodePtr->boundary = cptr->boundary;
    nodePtr->length = coord.length;
    return std::make_pair(span.first, nodePtr);
  }
  return std::make_pair(prevPair.first->second, prevPair.first->second);
}

void RnnIdContainer::addPath(const RnnIdResolver* resolver, const Lattice* lat,
                             const ConnectionPtr* cptr,
                             const ExtraNodesContext* xtra) {
  auto path = this->addPrevChain(resolver, lat, cptr, xtra);
  auto first = path.first;
  auto last = path.second;

  // publish found path (newly created nodes here)
  for (; first != last; last = last->prev) {
    auto& bnd = this->boundaries_[last->boundary];
    last->idx = bnd.nodeCnt;
    last->nextInBnd = bnd.node;
    bnd.node = last;
    bnd.nodeCnt += 1;
    addScore(last, cptr);
    auto& crd = resolveId(resolver, lat, cptr, xtra);
    crdCache_[crd] = last;
    cptr = cptr->previous;
  }
}

void RnnIdContainer::addScore(RnnNode* rnnNode, const ConnectionPtr* node) {
  auto score = this->alloc_->allocate<RnnScorePtr>();
  score->latPtr = node;
  score->rnn = rnnNode;
  auto& bnd = this->boundaries_[rnnNode->boundary];
  score->next = bnd.scores;
  bnd.scores = score;
  bnd.scoreCnt += 1;
}

const RnnCoordinate& RnnIdContainer::resolveId(const RnnIdResolver* resolver,
                                               const Lattice* lat,
                                               const ConnectionPtr* node,
                                               const ExtraNodesContext* xtra) {
  auto iter = nodeCache_.emplace(node->latticeNodePtr(), RnnCoordinate{});
  if (iter.second) {
    auto bnd = lat->boundary(node->boundary);
    auto starts = bnd->starts();
    auto& ninfo = starts->nodeInfo().at(node->right);
    auto eptr = ninfo.entryPtr();
    auto values = starts->entryData().row(node->right);
    auto repr = resolver->reprOf(&reprBldr_, eptr, values, xtra);
    if (eptr.isDic()) {
      auto t1 = resolver->knownIndex_.traversal();
      if (t1.step(repr) == dic::TraverseStatus::Ok) {
        iter.first->second.rnnId = t1.value();
      } else {
        iter.first->second.rnnId = resolver->unkId();
      }
    } else {
      auto t2 = resolver->unkIndex_.traversal();
      if (t2.step(repr) == dic::TraverseStatus::Ok) {
        iter.first->second.rnnId = t2.value();
      } else {
        iter.first->second.rnnId = resolver->unkId();
      }
    }
    iter.first->second.length = static_cast<u16>(ninfo.numCodepoints());
    iter.first->second.boundary = node->boundary;
  }
  return iter.first->second;
}

void RnnIdContainer::reset(u32 numBoundaries, u32 beamSize) {
  if (JUMANPP_USE_DEFAULT_ALLOCATION) {
    crdCache_.clear_no_resize();
    ptrCache_.clear_no_resize();
    nodeCache_.clear_no_resize();
    boundaries_.clear();
    boundaries_.resize(numBoundaries);
  } else {
    new (&crdCache_)
        util::FlatMap<RnnCoordinate, RnnNode*, RnnCrdHasher, RnnCrdHasher>{
            alloc_, numBoundaries * beamSize};
    new (&ptrCache_)
        util::FlatMap<ConnectionPtr*, RnnNode*, ConnPtrHasher, ConnPtrHasher>{
            alloc_, numBoundaries * beamSize / 2};
    new (&nodeCache_) util::FlatMap<LatticeNodePtr, RnnCoordinate>{
        alloc_, numBoundaries * beamSize / 2};
    new (&boundaries_)
        util::memory::ManagedVector<RnnBoundary>{numBoundaries, alloc_};
  }

  addBos();
  u16 eosPos = static_cast<u16>(numBoundaries - 1);
  nodeCache_[{eosPos, 0}] = RnnCoordinate{eosPos, 0, 0};
}

void RnnIdContainer::addBos() {
  auto bos0 = alloc_->allocate<RnnNode>();
  bos0->id = 0;
  bos0->idx = 0;
  bos0->boundary = 0;
  bos0->prev = nullptr;
  bos0->nextInBnd = nullptr;
  auto bos1 = alloc_->allocate<RnnNode>();
  bos1->hash = 0xdeadbeef0000;
  bos1->id = 0;
  bos1->idx = 0;
  bos1->boundary = 1;
  bos1->nextInBnd = nullptr;
  bos1->prev = bos0;
  boundaries_[1].node = bos1;
  boundaries_[1].nodeCnt = 1;
  RnnCoordinate bosCrd{1, 0, 0};
  crdCache_[bosCrd] = bos1;
  nodeCache_[{1, 0}] = bosCrd;
}

ConnectionPtr* RnnIdContainer::fakeConnection(
    const LatticeNodePtr& nodePtr, const ConnectionBeamElement* pElement,
    const BeamCandidate& candidate) {
  auto cptr = alloc_->allocate<ConnectionPtr>();
  cptr->previous = &pElement->ptr;
  cptr->boundary = nodePtr.boundary;
  cptr->right = nodePtr.position;
  cptr->left = candidate.left();
  cptr->beam = candidate.beam();
  return cptr;
}

}  // namespace rnn
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp
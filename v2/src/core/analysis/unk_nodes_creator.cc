//
// Created by Arseny Tolmachev on 2017/03/01.
//

#include "unk_nodes_creator.h"
#include "util/murmur_hash.h"

namespace jumanpp {
namespace core {
namespace analysis {

ChunkingUnkMaker::ChunkingUnkMaker(const dic::DictionaryEntries& entries_,
                                   chars::CharacterClass charClass_,
                                   UnkNodeConfig&& info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {}

bool ChunkingUnkMaker::spawnNodes(const AnalysisInput& input,
                                  UnkNodesContext* ctx,
                                  LatticeBuilder* lattice) const {
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto& codept = codepoints[i];
    if (!codept.hasClass(charClass_)) {
      continue;
    }

    auto trav = entries_.traversal();
    for (LatticePosition j = i; j < codepoints.size(); ++j) {
      auto& cp = codepoints[j];
      if (!cp.hasClass(charClass_)) {
        break;
      }
      auto status = trav.step(cp.bytes);

      switch (status) {
        case TraverseStatus::NoNode: {
          for (; j < codepoints.size(); ++j) {
            if (!codepoints[j].hasClass(charClass_)) {
              j = codepoints.size();
              break;
            }
            LatticePosition start = i;
            LatticePosition end = (LatticePosition)(j + 1);
            auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
            lattice->appendSeed(ptr, start, end);
          }
          break;
        }
        case TraverseStatus::NoLeaf: {
          LatticePosition start = i;
          LatticePosition end = (LatticePosition)(j + 1);
          auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
          lattice->appendSeed(ptr, start, end);
          break;
        }
        case TraverseStatus::Ok:
          continue;
      }
    }
  }
  return true;
}

SingleUnkMaker::SingleUnkMaker(const dic::DictionaryEntries& entries_,
                               chars::CharacterClass charClass_,
                               UnkNodeConfig&& info_)
    : entries_{entries_}, charClass_{charClass_}, info_{info_} {}

bool SingleUnkMaker::spawnNodes(const AnalysisInput& input,
                                UnkNodesContext* ctx,
                                LatticeBuilder* lattice) const {
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto& codept = codepoints[i];
    if (!codept.hasClass(charClass_)) {
      continue;
    } else {
      auto trav = entries_.traversal();
      auto result = trav.step(codept.bytes);
      bool notPrefix;
      switch (result) {
        case TraverseStatus::Ok:
          continue;
        case TraverseStatus::NoNode:
          notPrefix = true;
          break;
        case TraverseStatus::NoLeaf:
          notPrefix = false;
          break;
      }
      LatticePosition start = i;
      LatticePosition end = start;
      ++end;
      auto ptr = ctx->makePtr(codept.bytes, info_, notPrefix);
      lattice->appendSeed(ptr, start, end);
    }
  }
  return true;
}

OnomatopoeiaUnkMaker::OnomatopoeiaUnkMaker(const dic::DictionaryEntries &entries_,
                                           chars::CharacterClass charClass_,
                                           UnkNodeConfig&& info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {}

bool OnomatopoeiaUnkMaker::spawnNodes(const AnalysisInput &input,
                                      UnkNodesContext *ctx,
                                      LatticeBuilder *lattice) const {
  auto& codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto& cp1 = codepoints[i];
    if (!cp1.hasClass(charClass_)) {
      continue;
    }

    for (LatticePosition j = i + LatticePosition(2); j < codepoints.size(); ++j) {
      auto& cp2 = codepoints[j];
      if (!cp2.hasClass(charClass_)) {
        break;
      }

      if (cp1.codepoint == cp2.codepoint) {
        bool isOnomatopoeia = true;
        LatticePosition k = 1;
        for (; k < j - i && k < 5 && j + k < codepoints.size(); ++k) {
          if (codepoints[i + k].codepoint != codepoints[j + k].codepoint) {
            isOnomatopoeia = false;
            break;
          }
        }
        if (isOnomatopoeia) {
          LatticePosition start = i;
          LatticePosition end = j + k + LatticePosition(1);
          auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
          lattice->appendSeed(ptr, start, end);
        }
      }
    }
  }
  return true;
}

EntryPtr UnkNodesContext::makePtr(StringPiece surface,
                                  const UnkNodeConfig& conf, bool notPrefix) {
  auto node = xtra_->makeUnk(conf.base);
  auto data = xtra_->nodeContent(node);
  node->header.unk.surface = surface;
  i32 hashValue = hashUnkString(surface);
  node->header.unk.contentHash = hashValue;
  JPP_DCHECK_LT(node->header.unk.contentHash, 0);
  conf.fillElems(data, hashValue);
  if (conf.notPrefixIndex != -1) {
    xtra_->putPlaceholderData(node, conf.notPrefixIndex, (i32)notPrefix);
  }
  return node->ptr();
}

i32 hashUnkString(StringPiece sp) {
  constexpr u64 unkStringSeed = 0xa76210bfULL;
  u64 hash =
      util::hashing::murmurhash3_memory(sp.begin(), sp.end(), unkStringSeed);
  u32 trimmed = static_cast<u32>(hash);
  auto hashValue = static_cast<i32>(trimmed) | 0x8000'0000;
  return hashValue;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

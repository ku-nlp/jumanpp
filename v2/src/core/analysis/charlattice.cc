
#include "charlattice.h"
#include "util/logging.hpp"

namespace jumanpp {
namespace core {
namespace analysis {
namespace charlattice {

const char32_t DEF_PROLONG_SYMBOL1{U'ー'};
const char32_t DEF_PROLONG_SYMBOL2{U'〜'};
const char32_t DEF_PROLONG_SYMBOL3{U'っ'};
const char32_t DEF_PROLONG_SYMBOL4{U'ッ'};

struct CharDb {
  using Codepoint = jumanpp::chars::InputCodepoint;
  using CharacterClass = jumanpp::chars::CharacterClass;
  using CodepointStorage = std::vector<Codepoint>;
  using P = std::pair<char32_t, const char*>;

  inline static Codepoint toCodepoint(const char* str) {
    auto cp = Codepoint(StringPiece((const u8*)str, 3));
    return cp;
  }

  template <typename... T>
  static util::FlatMap<char32_t, Codepoint> codemapsfor(T... vals) {
    util::FlatMap<char32_t, Codepoint> result;
    std::initializer_list<std::pair<char32_t, const char*>> params = {
        P{vals}...};
    for (auto& elem : params) {
      result.insert(std::make_pair(elem.first, toCodepoint(elem.second)));
    }
    return result;
  }
  // Character Mappings
  const util::FlatMap<char32_t, Codepoint> lower2upper = codemapsfor(
      P(u'ぁ', "あ"), P(u'ぃ', "い"), P(u'ぅ', "う"), P(u'ぇ', "え"),
      P(u'ぉ', "お"), P(u'ゎ', "わ"), P(u'ヶ', "ケ"), P(u'ケ', "ヶ"));

  const util::FlatMap<char32_t, Codepoint> prolongedMap = codemapsfor(
      P(u'か', "あ"), P(u'が', "あ"), P(u'ば', "あ"), P(u'ま', "あ"),
      P(u'ゃ', "あ"), P(u'い', "い"), P(u'き', "い"), P(u'し', "い"),
      P(u'ち', "い"), P(u'に', "い"), P(u'ひ', "い"), P(u'じ', "い"),
      P(u'け', "い"), P(u'せ', "い"), P(u'へ', "い"), P(u'め', "い"),
      P(u'れ', "い"), P(u'げ', "い"), P(u'ぜ', "い"), P(u'で', "い"),
      P(u'べ', "い"), P(u'ぺ', "い"), P(u'く', "う"), P(u'す', "う"),
      P(u'つ', "う"), P(u'ふ', "う"), P(u'ゆ', "う"), P(u'ぐ', "う"),
      P(u'ず', "う"), P(u'ぷ', "う"), P(u'ゅ', "う"), P(u'お', "う"),
      P(u'こ', "う"), P(u'そ', "う"), P(u'と', "う"), P(u'の', "う"),
      P(u'ほ', "う"), P(u'も', "う"), P(u'よ', "う"), P(u'ろ', "う"),
      P(u'ご', "う"), P(u'ぞ', "う"), P(u'ど', "う"), P(u'ぼ', "う"),
      P(u'ぽ', "う"), P(u'ょ', "う"), P(u'え', "い"), P(u'ね', "い"));

  const util::FlatMap<char32_t, Codepoint> prolongedMapForErow = codemapsfor(
      P(u'え', "え"), P(u'け', "え"), P(u'げ', "え"), P(u'せ', "え"),
      P(u'ぜ', "え"), P(u'て', "え"), P(u'で', "え"), P(u'ね', "え"),
      P(u'へ', "え"), P(u'べ', "え"), P(u'め', "え"), P(u'れ', "え"));

  const util::FlatSet<char32_t> lowerList{u'ぁ', u'ぃ', u'ぅ', u'ぇ', u'ぉ'};

  const util::FlatMap<char32_t, char32_t> lowerMap{
      {u'か', u'ぁ'}, {u'さ', u'ぁ'}, {u'た', u'ぁ'}, {u'な', u'ぁ'},
      {u'は', u'ぁ'}, {u'ま', u'ぁ'}, {u'や', u'ぁ'}, {u'ら', u'ぁ'},
      {u'わ', u'ぁ'}, {u'が', u'ぁ'}, {u'ざ', u'ぁ'}, {u'だ', u'ぁ'},
      {u'ば', u'ぁ'}, {u'ぱ', u'ぁ'}, {u'い', u'ぃ'}, {u'し', u'ぃ'},
      {u'に', u'ぃ'}, {u'り', u'ぃ'}, {u'ぎ', u'ぃ'}, {u'じ', u'ぃ'},
      {u'ね', u'ぃ'}, {u'れ', u'ぃ'}, {u'ぜ', u'ぃ'}, {u'う', u'ぅ'},
      {u'く', u'ぅ'}, {u'す', u'ぅ'}, {u'ふ', u'ぅ'}, {u'む', u'ぅ'},
      {u'る', u'ぅ'}, {u'よ', u'ぅ'}, {u'け', u'ぇ'}, {u'せ', u'ぇ'},
      {u'て', u'ぇ'}, {u'め', u'ぇ'}, {u'れ', u'ぇ'}, {u'ぜ', u'ぇ'},
      {u'で', u'ぇ'}, {u'こ', u'ぉ'}, {u'そ', u'ぉ'}, {u'の', u'ぉ'},
      {u'も', u'ぉ'}, {u'よ', u'ぉ'}, {u'ろ', u'ぉ'}, {u'ぞ', u'ぉ'},
      {u'ど', u'ぉ'}};

  CharDb() = default;
};

const CharDb& CMaps() {
  static CharDb singleton_;
  return singleton_;
}

// Current char is prolong +
// Previous char is Kanji/Hiragana/Katakana
// or previous char was already deleted
inline bool isRemovableProlong(bool preIsDeleted,
                               const std::vector<Codepoint>& codepoints,
                               size_t pos) {
  if (pos < 1) return false;
  auto& cp = codepoints[pos];

  if (!cp.hasClass(CharacterClass::CHOON)) {
    return false;
  }

  CharacterClass preCharType = codepoints[pos - 1].charClass;

  if (preIsDeleted) {
    return true;
  }

  return IsCompatibleCharClass(preCharType, CharacterClass::FAMILY_PROLONGABLE);
}

// Remove if:
// * Self is one of small tsu
// * previous is removed
// * is last character
// * next is punctuation, alpha, digits
// * next is the same
// * previous and next are both either katakana or hiragana
inline bool CheckRemovableHatsuon(bool preIsDeleted,
                                  const std::vector<Codepoint>& codepoints,
                                  size_t pos) {
  // 直前が削除されていて、現在文字が"っ"、かつ、直後が文末もしくは記号の場合も削除
  if (pos == 0) return false;
  bool isLastCharacter = pos + 1 >= codepoints.size();
  const Codepoint& currentCp = codepoints[pos];

  if (currentCp.codepoint != DEF_PROLONG_SYMBOL3 &&
      currentCp.codepoint != DEF_PROLONG_SYMBOL4) {
    return false;
  }

  if (preIsDeleted) {
    return true;
  }

  if (isLastCharacter) {
    return true;
  }

  auto& nextCp = codepoints[pos + 1];
  CharacterClass nextCharType = nextCp.charClass;
  CharacterClass prevCharType = codepoints[pos - 1].charClass;

  const auto alwaysDeleteFor =
      CharacterClass::SPACE | CharacterClass::IDEOGRAPHIC_PUNC |
      CharacterClass::FIGURE | CharacterClass::PERIOD |
      CharacterClass::MIDDLE_DOT | CharacterClass::ALPH |
      CharacterClass::SYMBOL | CharacterClass::BRACKET | CharacterClass::SLASH |
      CharacterClass::COLON | CharacterClass::COMMA;

  if (IsCompatibleCharClass(nextCharType, alwaysDeleteFor)) {
    return true;
  }

  if (currentCp.codepoint == nextCp.codepoint) {
    return true;
  }

  auto commonTypes = nextCharType & prevCharType & currentCp.charClass;

  return IsCompatibleCharClass(commonTypes, CharacterClass::FAMILY_FULL_KANA);
}

inline bool CheckRemovableYouon(bool preIsDeleted,
                                const std::vector<Codepoint>& codepoints,
                                size_t pos) {
  /* 直前の文字が対応する平仮名の小書き文字、
   * ("かぁ" > "か(ぁ)" :()内は削除の意味)
   * または、削除された同一の小書き文字の場合は削除
   * ("か(ぁ)ぁ" > "か(ぁ)(ぁ)") */
  if (pos == 0) return false;
  const Codepoint& currentCp = codepoints[pos];
  const Codepoint& lastCp = codepoints[pos - 1];

  auto itr = CMaps().lowerMap.find(lastCp.codepoint);
  return (
      (itr != CMaps().lowerMap.end() && itr->second == currentCp.codepoint) ||
      (preIsDeleted && CMaps().lowerList.contains(currentCp.codepoint) &&
       currentCp.codepoint == lastCp.codepoint));
}

inline bool CheckSubstituteChoon(
    const std::vector<Codepoint>& codepoints, size_t pos,
    const util::FlatMap<char32_t, Codepoint>& map) {
  // (ーの文字 || 〜の文字)  && ２文字目以降
  if (pos > 0 && codepoints[pos].hasClass(CharacterClass::CHOON)) {
    auto itr = map.find(codepoints[pos - 1].codepoint);
    return (itr != map.end());
  } else
    return false;
}

inline bool CheckSubstituteLower(const std::vector<Codepoint>& codepoints,
                                 size_t pos) {
  auto itr = CMaps().lower2upper.find(codepoints[pos].codepoint);
  return (itr != CMaps().lower2upper.end());
}

CharLattceTraversal CharLattice::traversal(util::ArraySlice<Codepoint> input) {
  return CharLattceTraversal{*this, input};
}

int CharLattice::Parse(const std::vector<Codepoint>& codepoints) {
  size_t length = codepoints.size();
  bool preIsDeleted = false;
  bool nextPreIsDeleted = false;

  Codepoint skipped("");
  nodeList.clear();  // Lattice
  nodeList.reserve(length);
  for (int i = 0; i < length; ++i) {
    nodeList.emplace_back(alloc_);
  }

  const auto& db = CMaps();

  for (size_t pos = 0; pos < length; ++pos) {
    const Codepoint& currentCp = codepoints[pos];
    nextPreIsDeleted = false;

    /* Double Width Characters */
    if (currentCp.hasClass(CharacterClass::FAMILY_DOUBLE)) {
      /* Substitution */
      if (CheckSubstituteChoon(codepoints, pos, CMaps().prolongedMap)) {
        // Substitute choon to boin (ex. ねーさん > ねえさん)
        auto itr = db.prolongedMap.find(codepoints[pos - 1].codepoint);
        //        LOG_TRACE() << "<substitute_choon_boin> " << currentCp.bytes
        //        << ">"
        //                   << itr->second.bytes << "@" << pos;
        add(pos, itr->second, Modifiers::REPLACE | Modifiers::REPLACE_PROLONG);
        if (CheckSubstituteChoon(codepoints, pos,
                                 CMaps().prolongedMapForErow)) {
          auto it2 = db.prolongedMapForErow.find(codepoints[pos - 1].codepoint);
          add(pos, it2->second,
              Modifiers::REPLACE | Modifiers::REPLACE_PROLONG |
                  Modifiers::REPLACE_EROW_WITH_E);
        }
      } else if (CheckSubstituteLower(codepoints, pos)) {
        // Substitute lower character to upper character (ex. ねぇさん >
        // ねえさん)
        auto itr = db.lower2upper.find(currentCp.codepoint);
        //        LOG_TRACE() << "<substitute_small_large> " << currentCp.bytes
        //        << ">"
        //                   << itr->second.bytes << "@" << pos;
        add(pos, itr->second,
            Modifiers::REPLACE | Modifiers::REPLACE_SMALLKANA);
      }

      /* Deletion */
      if (isRemovableProlong(preIsDeleted, codepoints, pos)) {
        //        LOG_TRACE() << "<del_prolong> " << currentCp.bytes << "@" <<
        //        pos;
        add(pos, skipped, Modifiers::DELETE | Modifiers::DELETE_PROLONG);
        nextPreIsDeleted = true;
      } else if (CheckRemovableHatsuon(preIsDeleted, codepoints, pos)) {
        //        LOG_TRACE() << "<del_hatsu> " << currentCp.bytes << "@" <<
        //        pos;
        add(pos, skipped, Modifiers::DELETE | Modifiers::DELETE_HASTSUON);
        nextPreIsDeleted = true;
      } else if (CheckRemovableYouon(preIsDeleted, codepoints, pos)) {
        //        LOG_TRACE() << "<del_youon> @" << pos;
        add(pos, skipped, Modifiers::DELETE | Modifiers::DELETE_SMALLKANA);
        nextPreIsDeleted = true;
      }
    }
    preIsDeleted = nextPreIsDeleted;
  }
  constructed = true;
  return 0;
}

template <typename Iter, typename Comp, typename C>
Iter unique(Iter begin, Iter end, Comp comp, C& c) {
  switch (std::distance(begin, end)) {
    case 0:
    case 1:
      return end;
    default:;  // noop
  }
  Iter it = begin;
  ++it;
  while (it < end && !comp(*begin, *it)) {
    ++it;
    ++begin;
  }
  if (it == end) {
    return end;
  }
  for (; it < end; ++it) {
    JPP_DCHECK_LT(begin, it);
    while (it < end && comp(*begin, *it)) {
      c.push_back(std::move(*it));
      ++it;
    }
    if (it < end) {
      ++begin;
      *begin = std::move(*it);
    }
  }
  ++begin;
  return begin;
};

bool CharLattceTraversal::lookupCandidatesFrom(i32 start) {
  using dic::TraverseStatus;

  if (start > input_.length()) {
    return false;
  }
  result_.clear();
  auto trav = lattice_.entries.doubleArrayTraversal();
  auto stat = trav.step(input_.at(start).bytes);
  if (stat == TraverseStatus::NoNode) {
    return false;
  }
  auto posStart = static_cast<LatticePosition>(start);
  auto posEnd = static_cast<LatticePosition>(start + 1);
  auto state = make(trav, posStart, posEnd, Modifiers::ORIGINAL);
  state->lastStatus = stat;
  states1_.push_back(state);
  auto step = start + 1;
  while (step < input_.length() && !states1_.empty()) {
    doTraverseStep(step);
    ++step;
    states2_.swap(states1_);
    for (auto s : states2_) {
      buffer_.push_back(s);
    }
    states2_.clear();
    auto ptr = unique(states1_.begin(), states1_.end(),
                      [](const TraverasalState* s1, const TraverasalState* s2) {
                        return s1->end == s2->end &&
                               s1->allFlags == s2->allFlags &&
                               s1->traversal == s2->traversal;
                      },
                      buffer_);
    if (ptr != states1_.end()) {
      states1_.erase(ptr, states1_.end());
    }
  }
  for (auto& s : states1_) {
    buffer_.push_back(s);
  }
  states1_.clear();
  if (result_.empty()) {
    return false;
  }
  util::sort(result_, [](const CLResult& c1, const CLResult& c2) {
    auto v1 = c1.end == c2.end;
    if (v1) {
      return c1.entryPtr.rawValue() < c2.entryPtr.rawValue();
    }
    return c1.end < c2.end;
  });
  auto it = std::unique(result_.begin(), result_.end(),
                        [](const CLResult& c1, const CLResult& c2) {
                          // starts are equal at this point
                          return c1.entryPtr == c2.entryPtr && c1.end == c2.end;
                        });
  result_.erase(it, result_.end());
  return !result_.empty();
}

void CharLattceTraversal::doTraverseStep(i32 pos) {
  auto& ch = input_.at(pos);
  auto& extraNodes = lattice_.nodeList[pos];
  for (auto state : states1_) {
    if (state->end != pos) {
      // need to place a copy, because the memory will be recycled
      auto copy =
          make(state->traversal, state->start, state->end, state->allFlags);
      copy->allFlags = state->allFlags;
      states2_.push_back(copy);
    }
    tryWalk(state, ch, Modifiers::ORIGINAL, true);
    for (auto& n : extraNodes) {
      tryWalk(state, n.cp, n.type, !n.hasType(Modifiers::DELETE));
    }
  }
}

void CharLattceTraversal::tryWalk(TraverasalState* pState,
                                  const Codepoint& codepoint, Modifiers newFlag,
                                  bool doStep) {
  LatticePosition length{1};
  auto end = std::min<LatticePosition>(
      pState->end + length, static_cast<LatticePosition>(input_.size()));
  auto nst =
      make(pState->traversal, pState->start, end, pState->allFlags | newFlag);

  using dic::TraverseStatus;

  TraverseStatus status;
  if (doStep) {
    status = nst->traversal.step(codepoint.bytes);
  } else {
    status = pState->lastStatus;
  }

  if (status == TraverseStatus::NoNode) {
    buffer_.push_back(nst);  // return memory to pool
    return;
  }

  nst->lastStatus = status;

  if (status == TraverseStatus::Ok) {
    if (nst->allFlags != Modifiers::ORIGINAL) {  // we have a result!
      auto entries = this->lattice_.entries.entryTraversal(nst->traversal);
      auto flags = nst->allFlags;
      if (ExistFlag(newFlag, Modifiers::DELETE)) {
        flags = flags | Modifiers::DELETE_LAST;
      }
      i32 ptr;
      while (entries.readOnePtr(&ptr)) {
        result_.push_back({EntryPtr{ptr}, flags, nst->start, nst->end});
      }
    }
  }

  states2_.push_back(nst);
}

TraverasalState* CharLattceTraversal::make(const dic::DoubleArrayTraversal& t,
                                           LatticePosition start,
                                           LatticePosition end,
                                           Modifiers flags) {
  TraverasalState* data;
  if (buffer_.empty()) {
    data = lattice_.alloc_->make<TraverasalState>(t, start, end, flags);
  } else {
    data = buffer_.back();
    buffer_.pop_back();
    data->~TraverasalState();
    new (data) TraverasalState{t, start, end, flags};
  }
  return data;
}

}  // namespace charlattice
}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

//
// Created by morita on 2017/06/12.
//

#include "numeric_creator.h"
#include <iostream>

namespace jumanpp {
namespace core {
namespace analysis {

namespace {

struct NumericData {
  using Pattern = NumericUnkMaker::Pattern;
  using CodepointStorage = NumericUnkMaker::CodepointStorage;

  template <typename... T>
  static std::vector<const CodepointStorage> codeptsFor(T... vals) {
    std::vector<const CodepointStorage> result;
    std::initializer_list<StringPiece> params = {StringPiece{vals}...};
    for (auto &elem : params) {
      CodepointStorage pattern;
      preprocessRawData(elem, &pattern);
      result.emplace_back(std::move(pattern));
    }
    return result;
  }

  using S = StringPiece;

  // Exceptional Patterns
  const std::vector<const CodepointStorage> prefixPatterns =
      codeptsFor(S{"数"}, S{"何"}, S{"幾"});
  const std::vector<const CodepointStorage> interfixPatterns =
      codeptsFor("ぶんの", "分の");
  const std::vector<const CodepointStorage> suffixPatterns =
      codeptsFor("キロ", "メガ", "ギガ", "テラ", "ミリ");

  NumericData() noexcept {}
};

const NumericData &NUMERICS() {
  static NumericData singleton_;
  return singleton_;
}

}  // namespace

NumericUnkMaker::NumericUnkMaker(const dic::DictionaryEntries &entries_,
                                 chars::CharacterClass charClass_,
                                 UnkNodeConfig &&info_)
    : entries_(entries_), charClass_(charClass_), info_(info_) {
}

bool NumericUnkMaker::spawnNodes(const AnalysisInput &input,
                                 UnkNodesContext *ctx,
                                 LatticeBuilder *lattice) const {
  // Spawn the longest matting node
  auto &codepoints = input.codepoints();
  for (LatticePosition i = 0; i < codepoints.size(); ++i) {
    auto trav = entries_.traversal();
    LatticePosition nextstep = i;
    TraverseStatus status;

    auto length = FindLongestNumber(codepoints, i); // returns character length
    bool nonode = false;
    if (length > 0) {
      for (; nextstep < i + length; ++nextstep) {
        status = trav.step(codepoints[nextstep].bytes);
        if(status == TraverseStatus::NoNode){
          nonode = true;  
        }
      }

      if(nonode)
          status = TraverseStatus::NoNode;

      switch (status) {
        case TraverseStatus::NoNode: {  // 同一表層のノード無し prefix でもない
          LatticePosition start = i;
          LatticePosition end = i + length;
          auto ptr = ctx->makePtr(input.surface(start, end), info_, true);
          lattice->appendSeed(ptr, start, end);
          break;
        }
        case TraverseStatus::NoLeaf: {  // 先端
          LatticePosition start = i;
          LatticePosition end = i + length;
          auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
          lattice->appendSeed(ptr, start, end);
          break;
        }
        case TraverseStatus::Ok:
          LatticePosition start = i;
          LatticePosition end = i + length;
          auto ptr = ctx->makePtr(input.surface(start, end), info_, false);
          lattice->appendSeed(ptr, start, end);
      }
    }
  }
  return true;
}

size_t NumericUnkMaker::checkInterfix(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const {
  // In other words, check conditions for fractions ("５分の１" or "数ぶんの１")
  auto restLength = codepoints.size() - (start + pos);

  if (pos > 0) {  // 先頭ではない
    for (auto& itemp : NUMERICS().interfixPatterns) {

      if (  // Interfix follows a number.
          codepoints[start + pos - 1].hasClass(charClass_) &&
          // The length of rest codepoints is longer than the pattern length
          restLength > itemp.size() &&
          // A number follows interfix.
          codepoints[start + pos + itemp.size()].hasClass(charClass_)) {
        bool matchFlag = true;
        // The pattern matches the codepoints.
        for (u16 index = 0; index < itemp.size(); ++index) {
          if (codepoints[start + pos + index].codepoint !=
              itemp[index].codepoint) {
            matchFlag = false;
            break;
          }
        }
        if (matchFlag) return itemp.size();
      }
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkSuffix(const CodepointStorage &codepoints,
                                    LatticePosition start,
                                    LatticePosition pos) const {
  // In other words, check conditions for units ("５キロ" or "数ミリ")
  auto restLength = codepoints.size() - (start + pos);

  if (pos > 0) {
    for (auto& itemp : NUMERICS().suffixPatterns) {
      auto isException = codepoints[start + pos - 1].hasClass(ExceptionClass);
      if (  // Suffix follows a number.
          isException &&
          // The pattern length does not exceed that of rest codepoints.
          restLength >= itemp.size()) {
        // The pattern matches the codepoints.
        bool matchFlag = true;
        for (u16 index = 0; index < itemp.size(); ++index) {
          if (codepoints[start + pos + index].codepoint !=
              itemp[index].codepoint) {
            matchFlag = false;
            break;
          }
        }
        if (matchFlag) return itemp.size();
      }
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkPrefix(const CodepointStorage &codepoints,
                                    LatticePosition start,
                                    LatticePosition pos) const {
  // In other words, check conditions for units ("数十", "数ミリ", )

  for (auto &itemp : NUMERICS().prefixPatterns) {
    u16 suffixLength = 0;
    suffixLength = checkSuffix(codepoints, start, pos + itemp.size() );

    if (// The length of rest codepoints is longer than the pattern length
        start + pos + itemp.size() < codepoints.size() &&
        // A digit follows prefix.
        (codepoints[start + pos + itemp.size()].hasClass(DigitClass) || suffixLength >0)
          ) {
      bool matchFlag = true;
      // The pattern matches the codepoints.
      for (u16 index = 0; index < itemp.size(); ++index) {
        if (codepoints[start + pos + index].codepoint !=
            itemp[index].codepoint) {
          matchFlag = false;
          break;
        }
      }
      if (matchFlag) return itemp.size()+suffixLength;
    }
  }
  return 0;
}

size_t NumericUnkMaker::checkExceptionalCharsInFigure(
    const CodepointStorage &codepoints, LatticePosition start,
    LatticePosition pos) const {
  size_t charLength = 0;
  if ((charLength = checkPrefix(codepoints, start, pos)) > 0){
      std::cerr << "Prefix charLength" << charLength << std::endl;
      return charLength;
  }
  if ((charLength = checkInterfix(codepoints, start, pos)) > 0){
      std::cerr << "Interfix charLength" << charLength << std::endl;
    return charLength;
  }
  if ((charLength = checkSuffix(codepoints, start, pos)) > 0) {
      std::cerr << "Suffix charLength" << charLength << std::endl;
      return charLength;
  }
  if ((charLength = checkComma(codepoints, start, pos)) > 0) return charLength;
  if ((charLength = checkPeriod(codepoints, start, pos)) > 0) return charLength;
  return 0;
}

size_t NumericUnkMaker::checkComma(const CodepointStorage &codepoints,
                                   LatticePosition start,
                                   LatticePosition pos) const {
  // check exists of comma separated number beggining at pos.
  u16 numContinuedFigure = 0;
  u16 posComma = (start + pos);
  static const u16 commaInterval = 3;

  if (pos == 0) return 0;
  if (!codepoints[posComma].hasClass(CommaClass)) return 0;

  for (numContinuedFigure = 0;
       numContinuedFigure <= commaInterval + 1 &&
       posComma + 1 + numContinuedFigure < codepoints.size();
       ++numContinuedFigure) {
    if (! codepoints[posComma + 1 + numContinuedFigure].hasClass(FigureClass)) {
      break;
    }
  }
  if (numContinuedFigure == 3)
    return 1;
  else
    return 0;
}

size_t NumericUnkMaker::checkPeriod(const CodepointStorage &codepoints,
                                    LatticePosition start,
                                    LatticePosition pos) const {
  // check exists of comma separated number beggining at pos.
  unsigned int posPeriod = (start + pos);

  if (pos == 0) return 0;
  if (!codepoints[posPeriod].hasClass(PeriodClass)) 
      return 0;
  if (!codepoints[posPeriod - 1].hasClass(charClass_)) 
      return 0;
  if (pos + 1 < codepoints.size() && codepoints[posPeriod + 1].hasClass(charClass_))
      return 1;
  return 0;
}

size_t NumericUnkMaker::FindLongestNumber(const CodepointStorage &codepoints,
                                          LatticePosition start) const {
  LatticePosition pos = 0;
  for (pos = 0; pos <= MaxNumericLength && start + pos < codepoints.size();
       ++pos) {
    auto &cp = codepoints[start + pos];
    if (!cp.hasClass(charClass_)) {
      size_t charLength = 0;

      // Check Exception
      if ((charLength = checkExceptionalCharsInFigure(codepoints, start, pos)) >
          0) {
        pos += charLength -1; // pos is inclemented by for statement
      } else {
        return pos;
      }
    }
  }

  return pos;
}

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

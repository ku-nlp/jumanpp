//
// Created by morita on 2017/06/12.
//

#ifndef JUMANPP_NUMERIC_CREATOR_H
#define JUMANPP_NUMERIC_CREATOR_H

#include "unk_nodes_creator.h"

namespace jumanpp {
namespace core {
namespace analysis {

enum class NumericPattern : u16;

inline NumericPattern operator&(const NumericPattern &l,
                                const NumericPattern &r) {
  return static_cast<NumericPattern>(static_cast<u16>(l) & static_cast<u16>(r));
}

inline NumericPattern operator|(const NumericPattern &l,
                                const NumericPattern &r) {
  return static_cast<NumericPattern>(static_cast<u16>(l) | static_cast<u16>(r));
}

class NumericUnkMaker : public UnkMaker {
  dic::DictionaryEntries entries_;
  // A charcter class of Numbers (not include exceptions)
  chars::CharacterClass charClass_;
  UnkNodeConfig info_;

  using Pattern = NumericPattern;
  using CodepointStorage = std::vector<jumanpp::chars::InputCodepoint>;

  static const chars::CharacterClass FigureClass =
      chars::CharacterClass::FIGURE;
  static const chars::CharacterClass DigitClass =
      chars::CharacterClass::FIGURE_DIGIT;

  // Exceptional Classes
  static const chars::CharacterClass CommaClass = chars::CharacterClass::COMMA;
  static const chars::CharacterClass PeriodClass = chars::CharacterClass::FAMILY_NUM_PERIOD;

  // Exceptional Patterns
  const std::vector<std::string> prefixPatternsDef {"数", "何", "幾"};
  const std::vector<std::string> interfixPatternsDef {"ぶんの", "分の"};
  const std::vector<std::string> suffixPatternsDef {
      "キロ", "メガ", "ギガ", "テラ", "ミリ"};
  std::vector<CodepointStorage> prefixPatterns;
  std::vector<CodepointStorage> interfixPatterns;
  std::vector<CodepointStorage> suffixPatterns;

  static const size_t MaxNumericLength = 64;  //必要？

  size_t checkPrefix(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const;
  size_t checkInterfix(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const;
  size_t checkSuffix(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const;
  size_t checkComma(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const;
  size_t checkPeriod(const CodepointStorage &codepoints,
                                      LatticePosition start,
                                      LatticePosition pos) const;
  size_t checkExceptionalCharsInFigure(
    const CodepointStorage &codepoints, LatticePosition start,
    LatticePosition pos) const;


 public:
  NumericUnkMaker(const dic::DictionaryEntries &entries_,
                  chars::CharacterClass charClass_, UnkNodeConfig &&info_);

  bool spawnNodes(const AnalysisInput &input, UnkNodesContext *ctx,
                  LatticeBuilder *lattice) const override;

  size_t FindLongestNumber(const CodepointStorage &codepoints,
                     LatticePosition start) const;

  size_t check_exceptional_chars_in_figure(const CodepointStorage &codepoints,
                                           LatticePosition start) const;
};

}  // namespace analysis
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_NUMERIC_CREATOR_H

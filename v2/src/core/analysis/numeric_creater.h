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

enum class NumericPattern : u16 {
  None = 0,
  Normal = 1 << 2,         // 123456
  CammaSeparated = 1 << 3, // 123,456,789
  Fraction = 1 << 4,       // 3分の1
  WithUnit = 1 << 5,       // 3キロ
  Several = 1 << 6,        // 数十, 十数，

  All = Normal | CammaSeparated | Fraction | WithUnit;
};

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
  static const chars::CharacterClass CammaClass = chars::CharacterClass::COMMA;
  static const chars::CharacterClass PeriodClass =
      chars::CharacterClass::PERIOD | chars::CharacterClass::MIDDLE_DOT;

  // Exceptional Patterns
  static const std::vector<std::string> prefixPatternsDef = {"数", "何", "幾"};
  static const std::vector<std::string> interfixPatternsDef = {"ぶんの",
                                                               "分の"};
  static const std::vector<std::string> suffixPatternsDef = {
      "キロ", "メガ", "ギガ", "テラ", "ミリ"};
  std::vector<CodepointStorage> prefixPatterns;
  std::vector<CodepointStorage> interfixPatterns;
  std::vector<CodepointStorage> suffixPatterns;

  static const size_t MaxNumericLength = 64; //必要？

public:
  NumericUnkMaker(const dic::DictionaryEntries &entries_,
                  chars::CharacterClass charClass_, UnkNodeConfig &&info_);

  bool spawnNodes(const AnalysisInput &input, UnkNodesContext *ctx,
                  LatticeBuilder *lattice) const override;

  Pattern FindNumber(const CodepointStorage &codepoints,
                     LatticePosition start) const;

  size_t check_exceptional_chars_in_figure(const CodepointStorage &codepoints,
                                           LatticePosition start) const;
};

} // namespace analysis
} // namespace core
} // namespace jumanpp

#endif // JUMANPP_NUMERIC_CREATOR_H

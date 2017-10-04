//
// Created by Arseny Tolmachev on 2017/03/05.
//

#include "jumandic_spec.h"
#include "core/spec/spec_dsl.h"

namespace jumanpp {
namespace jumandic {

Status SpecFactory::makeSpec(core::spec::AnalysisSpec* spec) {
  core::spec::dsl::ModelSpecBuilder bldr;
  fillSpec(bldr);
  JPP_RETURN_IF_ERROR(bldr.build(spec));
  auto specColSize = spec->dictionary.columns.size();
  if (JumandicNumFields != specColSize) {
    return Status::InvalidState()
           << "please fix number of columns in jumandic::SpecFactory"
           << "right now spec has " << specColSize << " and SpecFactory has "
           << JumandicNumFields;
  }
  return Status::Ok();
}

//頂く,
// 0,
// 0,
// 0,
// 動詞,
// *,
// 基本形,
// 子音動詞カ行,
// 頂く,
// いただく,
// 頂く/いただく,
// 付属動詞候補（タ系） 謙譲動詞:貰う/もらう;食べる/たべる;飲む/のむ

void SpecFactory::fillSpec(core::spec::dsl::ModelSpecBuilder& bldr) {
  auto& surface = bldr.field(1, "surface").strings().trieIndex();
  auto& pos = bldr.field(5, "pos").strings().emptyValue("*");
  auto& subpos = bldr.field(6, "subpos").strings().emptyValue("*");
  auto& conjform = bldr.field(7, "conjform").strings().emptyValue("*");
  auto& conjtype = bldr.field(8, "conjtype").strings().emptyValue("*");
  auto& baseform = bldr.field(9, "baseform").strings().stringStorage(surface);
  auto& reading = bldr.field(10, "reading").strings().stringStorage(surface);
  /*auto& canonic = */ bldr.field(11, "canonic").strings().emptyValue("*");
  auto& features = bldr.field(12, "features").stringLists().emptyValue("NIL");

  auto& auxWord = bldr.feature("auxWord")
                      .matchAnyRowOfCsv("助詞\n助動詞\n判定詞", {pos})
                      .ifTrue({surface, pos, subpos})
                      .ifFalse({pos});
  auto& surfaceLength = bldr.feature("surfaceLength").numCodepoints(surface);
  auto& isDevoiced = bldr.feature("isDevoiced").matchData(features, "濁音化");
  auto& nominalize = bldr.feature("nominalize").matchData(features, "名詞化");
  auto& notPrefix = bldr.feature("notPrefix").placeholder();
  auto& lexicalized =
      bldr.feature("lexicalized")
          .matchAnyRowOfCsv(lexicalizedData, {baseform, pos, subpos, conjtype})
          .ifTrue({surface, pos, subpos, conjtype, conjform})
          .ifFalse({pos});

  bldr.unk("symbols", 1)
      .single(chars::CharacterClass::FAMILY_SYMBOL)
      .outputTo({surface, baseform, reading});
  bldr.unk("katakana", 2)
      .chunking(chars::CharacterClass::KATAKANA)
      .notPrefixOfDicFeature(notPrefix)
      .outputTo({surface, baseform, reading});
  bldr.unk("kanji", 3)
      .chunking(chars::CharacterClass::FAMILY_KANJI)
      .notPrefixOfDicFeature(notPrefix)
      .outputTo({surface, baseform, reading});
  bldr.unk("hiragana", 4)
      .chunking(chars::CharacterClass::HIRAGANA)
      .notPrefixOfDicFeature(notPrefix)
      .outputTo({surface, baseform, reading})
      .lowPriority();
  bldr.unk("alphabet", 5)
      .chunking(chars::CharacterClass::FAMILY_ALPH)
      .notPrefixOfDicFeature(notPrefix)
      .outputTo({surface, baseform, reading});
  bldr.unk("digits", 6)
      .numeric(chars::CharacterClass::FAMILY_DIGITS)
      .outputTo({surface, baseform, reading});

  bldr.unigram({surface});
  bldr.unigram({auxWord});
  bldr.unigram({pos});
  bldr.unigram({subpos});
  bldr.unigram({pos, subpos});
  bldr.unigram({conjtype});
  bldr.unigram({conjform});
  bldr.unigram({surfaceLength});
  bldr.unigram({surfaceLength, pos});
  bldr.unigram({surfaceLength, pos, subpos});
  bldr.unigram({baseform});
  bldr.unigram({baseform, pos});
  bldr.unigram({baseform, pos, subpos});
  bldr.unigram({isDevoiced});
  bldr.unigram({surfaceLength, notPrefix});
  bldr.unigram({baseform, notPrefix});
  bldr.unigram({pos, subpos, surfaceLength});
  bldr.unigram({nominalize});

  bldr.bigram({pos}, {pos});
  bldr.bigram({pos}, {pos, subpos});
  // 6 patterns
  bldr.bigram({pos, subpos}, {pos});
  bldr.bigram({pos, subpos}, {pos, subpos});
  bldr.bigram({pos, subpos}, {pos, subpos, conjtype});
  bldr.bigram({pos, subpos}, {pos, subpos, conjform});
  bldr.bigram({pos, subpos}, {pos, subpos, conjtype, conjform});
  bldr.bigram({pos, subpos}, {pos, subpos, conjtype, conjform, baseform});
  // 4 patterns
  bldr.bigram({pos, subpos, conjtype}, {pos, subpos});
  bldr.bigram({pos, subpos, conjtype}, {pos, subpos, conjtype});
  bldr.bigram({pos, subpos, conjtype}, {pos, subpos, conjform});
  bldr.bigram({pos, subpos, conjtype},
              {pos, subpos, conjtype, conjform, baseform});
  // 4 patterns
  bldr.bigram({pos, subpos, conjform}, {pos, subpos});
  bldr.bigram({pos, subpos, conjform}, {pos, subpos, conjtype});
  bldr.bigram({pos, subpos, conjform}, {pos, subpos, conjform});
  bldr.bigram({pos, subpos, conjform},
              {pos, subpos, conjtype, conjform, baseform});
  // 3 patterns
  bldr.bigram({pos, subpos, conjtype, conjform}, {pos, subpos});
  bldr.bigram({pos, subpos, conjtype, conjform},
              {pos, subpos, conjtype, conjform});
  bldr.bigram({pos, subpos, conjtype, conjform},
              {pos, subpos, conjtype, conjform, baseform});
  // 5 patterns
  bldr.bigram({pos, subpos, conjtype, conjform, baseform}, {pos, subpos});
  bldr.bigram({pos, subpos, conjtype, conjform, baseform},
              {pos, subpos, conjtype});
  bldr.bigram({pos, subpos, conjtype, conjform, baseform},
              {pos, subpos, conjform});
  bldr.bigram({pos, subpos, conjtype, conjform, baseform},
              {pos, subpos, conjtype, conjform});
  bldr.bigram({pos, subpos, conjtype, conjform, baseform},
              {pos, subpos, conjtype, conjform, baseform});
  // rest of bigrams
  bldr.bigram({lexicalized}, {lexicalized});
  bldr.bigram({baseform}, {baseform});
  bldr.bigram({surface}, {auxWord});
  bldr.bigram({auxWord}, {surface});
  bldr.bigram({subpos}, {subpos});
  bldr.bigram({conjform}, {conjform});  // check with morita-san
  bldr.bigram({subpos}, {pos});
  bldr.bigram({pos}, {subpos});
  bldr.bigram({conjform}, {pos});
  bldr.bigram({pos}, {conjform});

  bldr.trigram({pos}, {pos}, {pos});
  bldr.trigram({pos, subpos}, {pos, subpos}, {pos, subpos});
  bldr.trigram({pos, subpos, conjform}, {pos, subpos, conjform},
               {pos, subpos, conjform});
  bldr.trigram({lexicalized}, {lexicalized}, {lexicalized});

  bldr.train()
      .field(surface, 1.0f)
      .field(reading, 0)  // reading is ignored
      .field(baseform, 0.5f)
      .field(pos, 1.0f)
      .field(subpos, 1.0f)
      .field(conjtype, 0.5f)
      .field(conjform, 0.5f);
}

}  // namespace jumandic
}  // namespace jumanpp

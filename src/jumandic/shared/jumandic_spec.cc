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
  auto& surface = bldr.field(1, "surface").strings().trieIndex().align(4);
  auto& pos = bldr.field(5, "pos").strings().emptyValue("*").align(3);
  auto& subpos = bldr.field(6, "subpos").strings().emptyValue("*").align(3);
  auto& conjform = bldr.field(7, "conjform").strings().emptyValue("*").align(5);
  auto& conjtype = bldr.field(8, "conjtype").strings().emptyValue("*").align(4);
  auto& baseform = bldr.field(9, "baseform").strings().stringStorage(surface);
  auto& reading = bldr.field(10, "reading").strings().stringStorage(surface);
  /*auto& canonic = */ bldr.field(11, "canonic")
      .strings()
      .emptyValue("*")
      .align(3);
  auto& features = bldr.field(12, "features").kvLists().emptyValue("NIL");

  auto& auxWord = bldr.feature("auxWord")
                      .matchAnyRowOfCsv("助詞\n助動詞\n判定詞", {pos})
                      .ifTrue({surface, pos, subpos})
                      .ifFalse({pos});
  auto& surfaceLength = bldr.feature("surfaceLength").numCodepoints(surface);
  auto& isDevoiced = bldr.feature("isDevoiced").matchData(features, "濁音化D");
  auto& nominalize =
      bldr.feature("nominalize").matchData(features, "連用形名詞化");
  auto& notPrefix = bldr.feature("notPrefix").placeholder();
  auto& nonstdSurf = bldr.feature("nonstdSurf").placeholder();
  auto& lexicalized =
      bldr.feature("lexicalized")
          .matchAnyRowOfCsv(lexicalizedData, {baseform, pos, subpos, conjtype})
          .ifTrue({surface, pos, subpos, conjtype, conjform})
          .ifFalse({pos, subpos, conjtype});
  auto& scp1 = bldr.feature("scp1").codepoint(1);
  auto& scp2 = bldr.feature("scp2").codepoint(2);
  auto& scp3 = bldr.feature("scp3").codepoint(3);
  auto& sct1 = bldr.feature("sct1").codepointType(1);
  auto& sct0 = bldr.feature("sct0").codepointType(0);
  auto& sct1n = bldr.feature("sct1n").codepointType(-1);

  bldr.unk("symbols", 1)
      .single(chars::CharacterClass::FAMILY_SYMBOL)
      .outputTo({surface, baseform, reading});
  bldr.unk("katakana", 2)
      .chunking(chars::CharacterClass::KATAKANA)
      .writeFeatureTo(notPrefix)
      .outputTo({surface, baseform, reading});
  bldr.unk("kanji", 3)
      .chunking(chars::CharacterClass::FAMILY_KANJI)
      .writeFeatureTo(notPrefix)
      .outputTo({surface, baseform, reading});
  bldr.unk("hiragana", 4)
      .chunking(chars::CharacterClass::HIRAGANA)
      .writeFeatureTo(notPrefix)
      .outputTo({surface, baseform, reading})
      .lowPriority();
  bldr.unk("alphabet", 5)
      .chunking(chars::CharacterClass::FAMILY_ALPH)
      .writeFeatureTo(notPrefix)
      .outputTo({surface, baseform, reading});
  bldr.unk("digits", 6)
      .numeric(chars::CharacterClass::FAMILY_DIGITS)
      .outputTo({surface, baseform, reading});
  bldr.unk("normalize", 1)  // 1 because of API
      .normalize()
      .outputTo({surface})
      .writeFeatureTo(nonstdSurf);

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
  bldr.unigram({isDevoiced, pos, subpos});
  bldr.unigram({surfaceLength, notPrefix});
  bldr.unigram({baseform, notPrefix});
  bldr.unigram({pos, subpos, surfaceLength});
  bldr.unigram({nominalize});
  bldr.unigram({nonstdSurf});
  bldr.unigram({nonstdSurf, pos});
  bldr.unigram({nonstdSurf, pos, subpos});
  bldr.unigram({nonstdSurf, baseform});

  bldr.unigram({pos, subpos, conjform, conjtype, scp1});
  bldr.unigram({pos, subpos, conjform, conjtype, scp2});
  bldr.unigram({pos, subpos, conjform, conjtype, scp3});
  bldr.unigram({pos, subpos, conjform, conjtype, scp1, scp2});
  bldr.unigram({nonstdSurf, sct0, sct1});
  bldr.unigram({nonstdSurf, sct0, sct1n});

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
      .field(conjform, 0.5f)
      .allowGoldUnkWith(pos, features, "品詞推定");
}

}  // namespace jumandic
}  // namespace jumanpp

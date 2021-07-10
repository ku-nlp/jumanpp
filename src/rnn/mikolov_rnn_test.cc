//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "mikolov_rnn.h"
#include <util/memory.hpp>
#include "legacy/rnnlmlib_static.h"
#include "testing/standalone_test.h"
#include "util/logging.hpp"
#include "util/mmap.h"
#include "util/stl_util.h"

using namespace jumanpp;
using namespace jumanpp::rnn::mikolov;
using Catch::Matchers::WithinAbs;

TEST_CASE("can read mikonov rnn header") {
  util::MappedFile file;
  REQUIRE_OK(file.open("rnn/testlm.nnet", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(file.map(&frag, 0, file.size()));
  MikolovRnnModelHeader header;
  size_t offset = 0;
  REQUIRE_OK(readHeader(frag.asStringPiece(), &header, &offset));
  CHECK(offset != 0);
  CHECK(header.layerSize == 16);
  CHECK(header.nceLnz == 9);
  CHECK(header.maxentOrder == 3);
}

struct CtxAndScore {
  RNNLM_legacy::context ctx;
  float score;
};

class LegacyTester {
  RNNLM_legacy::CRnnLM_stat lm;

 public:
  LegacyTester() {
    lm.setDebugMode(0);
    lm.setRnnLMFile("rnn/testlm");
    lm.restoreNet_FR();
  }

  RNNLM_legacy::context initCtx() {
    RNNLM_legacy::context ctx;
    lm.get_initial_context_FR(&ctx);
    return ctx;
  }

  CtxAndScore step(RNNLM_legacy::context& ctx, const std::string& word) {
    RNNLM_legacy::context ctx2;
    float score = lm.test_word_selfnm(&ctx, &ctx2, word, word.size());
    return {ctx2, score};
  }
};

struct MyContext {
  util::MutableArraySlice<i32> prevWords{nullptr, 0};
  util::MutableArraySlice<float> context;
  float score;
  float normalizedScore;
  util::ArraySlice<float> allScores;

  float nScore(i32 idx) const {
    return std::log10(std::exp(allScores.at(idx)));
  }
};

struct MyMultiContext {};

template <typename T>
util::Sliceable<T> asSlice(util::MutableArraySlice<T> mas, i32 nRows = 1) {
  return util::Sliceable<T>{mas, mas.size() / nRows, (u32)nRows};
}

template <typename T>
util::Sliceable<T> asSliceX(T& o) {
  util::MutableArraySlice<T> mas{&o, 1};
  return util::Sliceable<T>{mas, 1, 1};
}

class MyTester {
 public:
  MikolovModelReader rdr;
  std::unordered_map<StringPiece, i32> s2i;
  MikolovRnn rnn;
  util::memory::Manager mgr{1024 * 1024};
  std::shared_ptr<util::memory::PoolAlloc> alloc;

 public:
  MyTester() : alloc{mgr.core()} {
    REQUIRE_OK(rdr.open("rnn/testlm"));
    REQUIRE_OK(rdr.parse());
    auto& strings = rdr.words();
    for (int i = 0; i < strings.size(); ++i) {
      auto& s = strings[i];
      s2i[s] = i;
    }
    REQUIRE_OK(rnn.init(rdr.header(), rdr.rnnMatrix(), rdr.maxentWeights()));
  }

  util::MutableArraySlice<i32> changeState(util::MutableArraySlice<i32> old,
                                           int one) {
    auto buf = alloc->allocateBuf<i32>(
        std::min<u32>(old.size() + 1, rdr.header().maxentOrder - 1));
    for (int i = 0; i < buf.size() - 1; ++i) {
      buf.at(i + 1) = old.at(i);
    }

    buf.front() = one;
    return buf;
  }

  std::vector<MyContext> stepBeam(std::initializer_list<MyContext> ctxs,
                                  std::initializer_list<StringPiece> data) {
    std::vector<i32> words;
    for (auto sp : data) {
      words.push_back(s2i.at(sp));
    }

    auto& h = rdr.header();
    util::MutableArraySlice<float> embedHack{
        const_cast<float*>(rdr.embeddings().data()), rdr.embeddings().size()};

    auto embeds = asSlice(embedHack, h.vocabSize);

    util::MutableArraySlice<float> nceHack{
        const_cast<float*>(rdr.nceEmbeddings().data()),
        rdr.embeddings().size()};

    auto nceSlice = asSlice(nceHack, h.vocabSize);

    auto prev = ctxs.begin()[0].prevWords[0];
    for (int i = 1; i < ctxs.size(); ++i) {
      REQUIRE(ctxs.begin()[i].prevWords[0] == prev);
      REQUIRE(ctxs.begin()[i].prevWords.size() ==
              ctxs.begin()[0].prevWords.size());
    }

    auto ctxData =
        alloc->allocate2d<i32>(ctxs.size(), ctxs.begin()[0].prevWords.size());
    auto ctxVecs = alloc->allocate2d<float>(ctxs.size(), h.layerSize, 64);
    for (int j = 0; j < ctxs.size(); ++j) {
      auto& ctx = ctxs.begin()[j];
      auto result = ctxData.row(j);
      util::copy_buffer(ctx.prevWords, result);
      auto r2 = ctxVecs.row(j);
      util::copy_buffer(ctx.context, r2);
    }

    auto nceVecs = alloc->allocate2d<float>(words.size(), h.layerSize, 64);
    for (int k = 0; k < words.size(); ++k) {
      auto res = nceVecs.row(k);
      util::copy_buffer(nceSlice.row(words[k]), res);
    }

    auto newCtxs = alloc->allocate2d<float>(ctxs.size(), h.layerSize, 64);
    auto scoreData = alloc->allocate2d<float>(ctxs.size(), words.size(), 64);
    util::fill(scoreData, 0);

    auto d = StepData{ctxData,          words,    ctxVecs,
                      embeds.row(prev),  // left embedding
                      nceVecs,           // right embedding
                      newCtxs,          scoreData};

    rnn.apply(&d);

    std::vector<MyContext> result;
    for (int l = 0; l < ctxs.size(); ++l) {
      result.push_back({changeState(ctxData.row(l), prev),  // state
                        newCtxs.row(l), 0.f, 0.f, scoreData.row(l)});
    }

    return result;
  }

  std::vector<MyContext> stepParallel(std::initializer_list<MyContext> ctxs,
                                      std::initializer_list<StringPiece> data) {
    REQUIRE(ctxs.size() == data.size());

    std::vector<i32> words;
    for (auto sp : data) {
      words.push_back(s2i.at(sp));
    }

    auto& h = rdr.header();
    util::MutableArraySlice<float> embedHack{
        const_cast<float*>(rdr.embeddings().data()), rdr.embeddings().size()};
    auto embeds = asSlice(embedHack, h.vocabSize);

    util::MutableArraySlice<float> nceHack{
        const_cast<float*>(rdr.nceEmbeddings().data()),
        rdr.embeddings().size()};

    auto nceSlice = asSlice(nceHack, h.vocabSize);

    auto ctxData =
        alloc->allocate2d<i32>(ctxs.size(), ctxs.begin()[0].prevWords.size());
    auto ctxVecs = alloc->allocate2d<float>(ctxs.size(), h.layerSize, 64);
    for (int j = 0; j < ctxs.size(); ++j) {
      auto& ctx = ctxs.begin()[j];
      auto result = ctxData.row(j);
      util::copy_buffer(ctx.prevWords, result);
      auto r2 = ctxVecs.row(j);
      util::copy_buffer(ctx.context, r2);
    }

    auto nceVecs = alloc->allocate2d<float>(words.size(), h.layerSize, 64);
    auto leftEmbeds = alloc->allocate2d<float>(words.size(), h.layerSize, 64);
    for (int k = 0; k < words.size(); ++k) {
      auto res = nceVecs.row(k);
      util::copy_buffer(nceSlice.row(words[k]), res);
      auto embBuf = leftEmbeds.row(k);
      auto prevId = ctxs.begin()[k].prevWords.at(0);
      util::copy_buffer(embeds.row(prevId), embBuf);
    }

    auto newCtxs = alloc->allocate2d<float>(ctxs.size(), h.layerSize, 64);
    auto scoreData = alloc->allocateBuf<float>(words.size(), 64);
    util::fill(scoreData, 0);

    ParallelContextData pcd{ctxVecs, leftEmbeds, newCtxs};
    rnn.computeNewParCtx(&pcd);

    ParallelStepData psd{ctxData, words, newCtxs, nceVecs, scoreData};
    rnn.applyParallel(&psd);
    std::vector<MyContext> result;
    for (u32 l = 0; l < ctxs.size(); ++l) {
      result.push_back({changeState(ctxData.row(l), words.at(l)),  // state
                        newCtxs.row(l),
                        0.f,
                        0.f,
                        {scoreData, l, 1}});
    }
    return result;
  }

  MyContext step(MyContext& ctx, StringPiece str) {
    int wordId = s2i.at(str);

    auto& h = rdr.header();

    util::MutableArraySlice<float> embedHack{
        const_cast<float*>(rdr.embeddings().data()), rdr.embeddings().size()};

    auto embeds = asSlice(embedHack, h.vocabSize);

    util::MutableArraySlice<float> nceHack{
        const_cast<float*>(rdr.nceEmbeddings().data()),
        rdr.embeddings().size()};

    auto nceSlice = asSlice(nceHack, h.vocabSize);

    util::Sliceable<float> nceItem{nceSlice.row(wordId), h.layerSize, 1};

    auto scoreData = alloc->allocateBuf<float>(1, 64);
    scoreData[0] = 0;

    auto ctx2 = makeCtx();
    auto d = StepData{asSlice(ctx.prevWords),
                      asSliceX(wordId).data(),
                      asSlice(ctx.context),
                      embeds.row(ctx.prevWords[0]),  // left embedding
                      nceItem,                       // right embedding
                      asSlice(ctx2.context),
                      asSlice(scoreData)};

    rnn.apply(&d);

    ctx2.score = scoreData.at(0);
    ctx2.normalizedScore = std::log10(std::exp(ctx2.score));
    ctx2.prevWords = changeState(ctx.prevWords, wordId);

    return ctx2;
  }

  MyContext makeCtx() {
    auto& h = rdr.header();
    MyContext ctx;
    ctx.context = alloc->allocateBuf<float>(h.layerSize, 64);
    util::fill(ctx.context, 0);
    return ctx;
  }

  MyContext zero() {
    auto ct = makeCtx();
    ct.prevWords = alloc->allocateBuf<i32>(1);
    ct.prevWords[0] = 0;
    return ct;
  }
};

TEST_CASE("legacy rnn reader works") {
  util::logging::CurrentLogLevel = util::logging::Level::Info;
  LegacyTester ltst;
  auto init = ltst.initCtx();
  auto c1 = ltst.step(init, "me");
  CHECK_THAT(c1.score, WithinAbs(-4.22612f, 1e-3f));
  auto c2 = ltst.step(c1.ctx, "what");
  CHECK_THAT(c2.score, WithinAbs(-2.78254f, 1e-3f));
  auto c3 = ltst.step(c2.ctx, "is");
  auto c4 = ltst.step(c3.ctx, "this");
  auto c5 = ltst.step(c4.ctx, "thing");
  CHECK_THAT(c5.score, WithinAbs(-2.58717f, 1e-3f));
}

TEST_CASE("model reader can read vocabulary") {
  MyTester tstr;
  CHECK(tstr.rdr.words().size() == 10000);
}

TEST_CASE("reimplementation can compute scores") {
  MyTester tstr;
  CHECK(tstr.rdr.words().size() == 10000);
  auto z = tstr.zero();
  auto s1 = tstr.step(z, "me");
  CHECK_THAT(s1.normalizedScore, WithinAbs(-4.22612f, 1e-3f));
  CHECK(s1.prevWords[0] == 811);
  CHECK(s1.prevWords[1] == 0);
  auto s2 = tstr.step(s1, "what");
  CHECK_THAT(s2.normalizedScore, WithinAbs(-2.78254f, 1e-3f));
  auto s3 = tstr.step(s2, "is");
  auto s4 = tstr.step(s3, "this");
  auto s5 = tstr.step(s4, "thing");
  CHECK_THAT(s5.normalizedScore, WithinAbs(-2.58717f, 1e-3f));
}

TEST_CASE("legacy rnn computes scores for us") {
  util::logging::CurrentLogLevel = util::logging::Level::Info;
  LegacyTester ltst;
  auto init = ltst.initCtx();
  auto a1 = ltst.step(init, "help");
  auto a2 = ltst.step(init, "save");
  auto b1 = ltst.step(a1.ctx, "me");
  auto b2 = ltst.step(a2.ctx, "me");
  auto c11 = ltst.step(b1.ctx, "when");
  auto c12 = ltst.step(b1.ctx, "duck");
  auto c13 = ltst.step(b1.ctx, "company");
  auto c21 = ltst.step(b2.ctx, "when");
  auto c22 = ltst.step(b2.ctx, "duck");
  auto c23 = ltst.step(b2.ctx, "company");
  CHECK_THAT(c11.score, WithinAbs(-2.81244f, 1e-3f));
  CHECK_THAT(c12.score, WithinAbs(-5.74136f, 1e-3f));
  CHECK_THAT(c13.score, WithinAbs(-3.71568f, 1e-3f));
  CHECK_THAT(c21.score, WithinAbs(-2.4811f, 1e-3f));
  CHECK_THAT(c22.score, WithinAbs(-6.68284f, 1e-3f));
  CHECK_THAT(c23.score, WithinAbs(-3.62682f, 1e-3f));
}

TEST_CASE("repimplementation works in NxM mode") {
  MyTester tstr;
  auto z = tstr.zero();
  auto a1 = tstr.step(z, "help");
  auto a2 = tstr.step(z, "save");
  auto b1 = tstr.step(a1, "me");
  auto b2 = tstr.step(a2, "me");
  auto c = tstr.stepBeam({b1, b2}, {"when", "duck", "company"});
  auto d = tstr.step(b1, "when");
  auto d2 = tstr.step(b2, "company");
  CHECK_THAT(d.normalizedScore, WithinAbs(-2.81244f, 1e-3f));
  CHECK(c.size() == 2);
  auto& c0 = c[0];
  auto& c1 = c[1];
  CHECK_THAT(c0.nScore(0), WithinAbs(d.normalizedScore, 1e-3f));
  CHECK_THAT(c1.nScore(2), WithinAbs(d2.normalizedScore, 1e-3f));
  CHECK_THAT(c0.nScore(0), WithinAbs(-2.81244f, 1e-3f));
  CHECK_THAT(c0.nScore(1), WithinAbs(-5.74136f, 1e-3f));
  CHECK_THAT(c0.nScore(2), WithinAbs(-3.71568f, 1e-3f));
  CHECK_THAT(c1.nScore(0), WithinAbs(-2.4811f, 1e-3f));
  CHECK_THAT(c1.nScore(1), WithinAbs(-6.68284f, 1e-3f));
  CHECK_THAT(c1.nScore(2), WithinAbs(-3.62682f, 1e-3f));
}

TEST_CASE("reimplementation works in parallel mode with a single arg") {
  MyTester tstr;
  auto z = tstr.zero();
  auto a1 = tstr.step(z, "help");
  auto b = tstr.stepParallel({a1}, {"me"});
  auto b1a = tstr.step(a1, "me");
  REQUIRE(b.size() == 1);
  CHECK_THAT(b[0].nScore(0), WithinAbs(b1a.normalizedScore, 1e-4f));
}

TEST_CASE("reimplementation works in parallel mode") {
  MyTester tstr;
  auto z = tstr.zero();
  auto a1 = tstr.step(z, "help");
  auto a2 = tstr.step(z, "save");
  auto b = tstr.stepParallel({a1, a2}, {"me", "me"});
  auto b1a = tstr.step(a1, "me");
  auto b2a = tstr.step(a2, "me");
  REQUIRE(b.size() == 2);
  CHECK_THAT(b[0].nScore(0), WithinAbs(b1a.normalizedScore, 1e-3f));
  CHECK_THAT(b[1].nScore(0), WithinAbs(b2a.normalizedScore, 1e-3f));
}
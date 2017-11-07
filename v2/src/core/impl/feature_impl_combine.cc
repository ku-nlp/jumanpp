//
// Created by Arseny Tolmachev on 2017/10/23.
//

#include "feature_impl_combine.h"
#include "util/stl_util.h"

namespace jumanpp {
namespace core {
namespace features {
namespace impl {

namespace i = util::io;

#if 0

bool PatternDynamicApplyImpl::printClassBody(util::io::Printer& p) {
  p << "\nvoid apply("  // args
    << JPP_TEXT(::jumanpp::util::ArraySlice<::jumanpp::u64>) << " features, "
    << JPP_TEXT(::jumanpp::util::MutableArraySlice<::jumanpp::u64>) << " result"
    << ") const noexcept {";

  auto childrenCopy = children;
  util::sort(childrenCopy, [](const DynamicPatternFeatureImpl& f1,
                              const DynamicPatternFeatureImpl& f2) {
    return f1.index() < f2.index();
  });

  i::Indent ind{p, 2};
  for (int i = 0; i < childrenCopy.size(); ++i) {
    p << "\n{";
    i::Indent ind2{p, 2};
    auto& c = childrenCopy[i];
    p << "\nstatic constexpr " << JPP_TEXT(::jumanpp::i32) << " args_" << i
      << "[] = {";
    for (auto& v : c.arguments()) {
      p << v << ", ";
    }
    p << "};";
    p << "\nconstexpr "
      << JPP_TEXT(::jumanpp::core::features::impl::DynamicPatternFeatureImpl)
      << " feat_" << i << " { " << c.index() << ", args_" << i << "};";
    p << "\nfeat_" << i << ".apply(features, result);";
    p << "\n}";
  }

  p << "\n}";
  return true;
}

#endif

Status NgramDynamicFeatureApply::addChild(
    const spec::NgramFeatureDescriptor& nf) {
  switch (nf.references.size()) {
    case 1: {
      children.emplace_back(
          new NgramFeatureDynamicAdapter<1>{nf.index, nf.references[0]});
      break;
    }
    case 2: {
      children.emplace_back(new NgramFeatureDynamicAdapter<2>{
          nf.index, nf.references[0], nf.references[1]});
      break;
    }
    case 3: {
      children.emplace_back(new NgramFeatureDynamicAdapter<3>{
          nf.index, nf.references[0], nf.references[1], nf.references[2]});
      break;
    }
    default:
      return JPPS_INVALID_STATE << "invalid ngram feature of order "
                                << nf.references.size()
                                << " only 1-3 are supported";
  }
  return Status::Ok();
}
}  // namespace impl
}  // namespace features
}  // namespace core
}  // namespace jumanpp
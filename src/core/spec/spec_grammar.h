//
// Created by Arseny Tolmachev on 2017/02/22.
//

#ifndef JUMANPP_SPEC_FIELD_PARSER_H
#define JUMANPP_SPEC_FIELD_PARSER_H

#define TAO_PEGTL_NAMESPACE jumanpp_pegtl

#include <pegtl/ascii.hpp>
#include <pegtl/rules.hpp>

namespace jumanpp {
namespace core {
namespace spec {
namespace parser {

namespace p = ::tao::jumanpp_pegtl;
using p::istring;
using p::opt;
using p::plus;
using p::seq;

struct qstring {
  using analyze_t = p::analysis::generic<p::analysis::rule_type::ANY>;

  template <typename Input>
  static bool match(Input& in) {
    if (in.size() < 2) return false;
    auto ptr = in.current();
    if (*ptr != '"') return false;
    if (in.size() == 2 && *(ptr + 1) == '"') {
      // empty string case
      in.bump(2);
      return true;
    }
    ++ptr;  // skip first char
    for (; ptr < in.end(); ++ptr) {
      if (*ptr == '"') {
        auto nextChar = ptr + 1;
        if ((nextChar == in.end()) ||            // we are at the end of input
            (*nextChar != '"')) {                // or next symbol is not quote
          auto strlen = ptr - in.current() + 1;  // +1 for the second quote
          in.bump(strlen);
          return true;
        } else {
          ++ptr;
        }
      }
    }
    return false;
  }
};

#ifdef __JETBRAINS_IDE__
#define lit_string(x) p::istring<'x'>
#else
#define lit_string(x) TAO_PEGTL_ISTRING(x)
#endif

struct whitespace : plus<p::blank> {};
struct opt_whitespace : p::star<p::blank> {};
struct lbrak : p::one<'['> {};
struct rbrak : p::one<']'> {};
struct comma : p::one<','> {};
struct lparen : p::one<'('> {};
struct rparen : p::one<')'> {};
struct eq : p::one<'='> {};
struct add : lit_string("+=") {};

struct ident : p::identifier {};
struct number : p::rep_min_max<1, 5, p::digit> {};

struct kw_field : lit_string("field") {};
struct kw_unk : lit_string("unknown") {};
struct kw_feature : lit_string("feature") {};
struct kw_unigram : lit_string("unigram") {};
struct kw_bigram : lit_string("bigram") {};
struct kw_trigram : lit_string("trigram") {};
struct kw_data : lit_string("data") {};
struct kw_if : lit_string("if") {};
struct kw_else : lit_string("else") {};
struct kw_match : lit_string("match") {};
struct kw_output : lit_string("output") {};
struct kw_template : lit_string("template") {};

struct comment : seq<p::one<'#'>, p::until<p::eol>> {};
struct sep : p::pad_opt<comment, whitespace> {};
struct sep_pad : p::sor<whitespace, comment> {};

struct strparam : ident {};
struct qstring_param : qstring {};

struct fld_flag_index : lit_string("trie_index") {};
struct fld_flag_empty_kw : lit_string("empty") {};
struct fld_flag_empty : p::if_must<fld_flag_empty_kw, sep, qstring_param> {};
struct fld_flag_storage_kw : lit_string("storage") {};
struct fld_flag_storage : p::if_must<fld_flag_storage_kw, sep, strparam> {};
struct fld_flag_align_kw : lit_string("align") {};
struct fld_flag_align_data : number {};
struct fld_flag_align
    : p::if_must<fld_flag_align_kw, sep, fld_flag_align_data> {};
struct fld_flag_sep : p::if_must<lit_string("list_sep"), sep, qstring_param> {};
struct fld_flag_kvsep : p::if_must<lit_string("kv_sep"), sep, qstring_param> {};
struct fld_flag : p::sor<fld_flag_empty, fld_flag_index, fld_flag_storage,
                         fld_flag_align, fld_flag_sep, fld_flag_kvsep> {};
struct fld_flags : p::star<sep, fld_flag> {};
struct fld_tp_slist : lit_string("string_list") {};
struct fld_tp_kvlist : lit_string("kv_list") {};
struct fld_tp_string : lit_string("string") {};
struct fld_tp_int : lit_string("int") {};
struct fld_type
    : p::sor<fld_tp_slist, fld_tp_kvlist, fld_tp_string, fld_tp_int> {};
struct fld_column : number {};
struct fld_name : ident {};
struct fld_data : p::seq<fld_column, sep, fld_name, sep, fld_type, fld_flags> {
};

struct fld_stmt : p::if_must<kw_field, sep, fld_data> {};

struct fieldparam : ident {};
struct snumparam : p::seq<p::opt<p::one<'-'>>, number> {};

struct ft_codepoint : p::if_must<lit_string("codepoint"), sep, snumparam> {};
struct ft_codepoint_type
    : p::if_must<lit_string("codepoint_type"), sep, snumparam> {};
struct ft_num_codepts
    : p::if_must<lit_string("num_codepoints"), sep, fieldparam> {};
struct ft_placeholder : lit_string("placeholder") {};
struct ft_byte_length : p::if_must<lit_string("num_bytes"), sep, fieldparam> {};

struct mt_lhs_litem : p::seq<fieldparam> {};
struct mt_lhs_list
    : p::if_must<lbrak, sep, p::list<mt_lhs_litem, comma, sep_pad>, sep,
                 rbrak> {};
struct mt_lhs : p::sor<mt_lhs_litem, mt_lhs_list> {};
struct mt_rhs_file : p::seq<lit_string("file"), sep, qstring_param> {};
struct mt_rhs : p::sor<qstring_param, mt_rhs_file> {};
struct mt_cond : p::seq<mt_lhs, sep, lit_string("with"), sep, mt_rhs> {};
struct mt_then_body : p::seq<lit_string("then"), sep, mt_lhs_list> {};
struct mt_else_body : p::seq<lit_string("else"), sep, mt_lhs_list> {};
struct ft_match : p::if_must<lit_string("match"), sep, mt_cond,
                             p::opt<sep, mt_then_body, sep, mt_else_body>> {};

struct feature_body : p::sor<ft_codepoint_type, ft_codepoint, ft_num_codepts,
                             ft_placeholder, ft_byte_length, ft_match> {};
struct feature_name : ident {};
struct feature_stmt : p::if_must<lit_string("feature"), sep, feature_name,
                                 p::pad_opt<eq, sep_pad>, feature_body> {};

struct fref_item : ident {};
struct fref_list
    : p::if_must<lbrak, sep, p::list<fref_item, comma, sep_pad>, sep, rbrak> {};
struct ngram_uni : p::if_must<lit_string("unigram"), sep, fref_list> {};

struct arg_name : ident {};
struct arg_op : p::sor<eq, add> {};
struct arg_par : p::sor<number, qstring> {};
struct arg_action : p::if_must<arg_op, sep, arg_par> {};
struct arg : p::seq<arg_name, sep, opt<arg_action>> {};

struct arglist_content : p::list_must<arg, comma, p::blank> {};
struct arglist : p::if_must<lbrak, sep, arglist_content, sep, rbrak> {};

struct unk_f_out_data : arglist {};
struct unk_f_out : p::if_must<kw_output, sep, unk_f_out_data> {};
struct unk_f_feat_data : arglist {};
struct unk_f_feat : p::if_must<kw_feature, sep, unk_f_feat_data> {};
struct unk_tmpl_num : number {};
struct unk_name : ident {};
struct unk_data : p::if_must<kw_unk, sep, unk_name, sep, kw_template, sep,
                             unk_tmpl_num, sep, unk_f_out, sep, unk_f_feat> {};

struct mtch_lhs : ident {};
struct mtch_arglist : arglist {};
struct mtch_str : qstring {};
struct mtch_rhs : p::sor<mtch_str, mtch_arglist> {};
struct mtch : p::must<mtch_lhs, sep, kw_match, sep, mtch_rhs> {};

struct if_cond_body : mtch {};
struct if_cond : p::if_must<lparen, sep, if_cond_body, sep, rparen> {};
struct if_true : arglist {};
struct if_false : arglist {};
struct if_expr : p::if_must<kw_if, sep, if_cond, sep, if_true, sep, kw_else,
                            sep, if_false> {};

struct feat_bdy_if : if_expr {};
struct feat_bdy_eq : arg {};
struct feat_bdy : p::sor<feat_bdy_if, feat_bdy_eq> {};
struct feat_name : ident {};
struct feat_data : p::if_must<kw_feature, sep, feat_name, sep, feat_bdy> {};

struct uni_lst : arglist {};
struct uni_data : p::if_must<kw_unigram, sep, uni_lst> {};

struct bi_lst_1 : arglist {};
struct bi_lst_2 : arglist {};
struct bi_data : p::if_must<kw_bigram, sep, bi_lst_1, sep, bi_lst_2> {};

struct tri_list_1 : arglist {};
struct tri_list_2 : arglist {};
struct tri_list_3 : arglist {};
struct tri_data : p::if_must<kw_trigram, sep, tri_list_1, sep, tri_list_2, sep,
                             tri_list_3> {};

#undef lit_string

}  // namespace parser
}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_FIELD_PARSER_H

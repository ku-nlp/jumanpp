//
// Created by Arseny Tolmachev on 2017/02/22.
//

#ifndef JUMANPP_SPEC_FIELD_PARSER_H
#define JUMANPP_SPEC_FIELD_PARSER_H

#include <pegtl/ascii.hh>
#include <pegtl/rules.hh>

namespace jumanpp {
namespace core {
namespace spec {
namespace parser {

namespace p = pegtl;
using p::plus;
using p::opt;
using p::istring;
using p::seq;

struct qstring {
  using analyze_t = pegtl::analysis::generic<pegtl::analysis::rule_type::ANY>;

  template <pegtl::apply_mode A, template <typename...> class Action,
            template <typename...> class Control, typename... States>
  static bool match(pegtl::input& in, States&...) {
    if (in.size() < 2) return false;
    auto ptr = in.begin();
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
        if ((nextChar == in.end()) ||          // we are at the end of input
            (*nextChar != '"')) {              // or next symbol is not quote
          auto strlen = ptr - in.begin() + 1;  // +1 for the second quote
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

struct whitespace : plus<pegtl::blank> {};
struct opt_whitespace : p::star<pegtl::blank> {};
struct lbrak : p::one<'['> {};
struct rbrak : p::one<']'> {};
struct comma : p::one<','> {};
struct lparen : p::one<'('> {};
struct rparen : p::one<')'> {};
struct eq : p::one<'='> {};
struct add : pegtl_istring_t("+=") {};

struct ident : p::identifier {};
struct number : p::rep_min_max<1, 5, p::digit> {};

struct kw_field : pegtl_istring_t("field") {};
struct kw_unk : pegtl_istring_t("unknown") {};
struct kw_feature : pegtl_istring_t("feature") {};
struct kw_unigram : pegtl_istring_t("unigram") {};
struct kw_bigram : pegtl_istring_t("bigram") {};
struct kw_trigram : pegtl_istring_t("trigram") {};
struct kw_data : pegtl_istring_t("data") {};
struct kw_if : pegtl_istring_t("if") {};
struct kw_else : pegtl_istring_t("else") {};
struct kw_match : pegtl_istring_t("match") {};
struct kw_output : pegtl_istring_t("output") {};
struct kw_template : pegtl_istring_t("template") {};

struct sep : opt_whitespace {};

struct fld_flag_index : pegtl_istring_t("trie_index") {};
struct fld_flag_empty_kw : pegtl_istring_t("empty") {};
struct fld_flag_empty_data : qstring {};
struct fld_flag_empty
    : p::if_must<fld_flag_empty_kw, sep, fld_flag_empty_data> {};
struct fld_flag : p::sor<fld_flag_empty, fld_flag_index> {};
struct fld_flags : p::star<sep, fld_flag> {};
struct fld_tp_slist : pegtl_istring_t("string_list") {};
struct fld_tp_string : pegtl_istring_t("string") {};
struct fld_tp_int : pegtl_istring_t("int") {};
struct fld_type : p::sor<fld_tp_slist, fld_tp_string, fld_tp_int> {};
struct fld_column : number {};
struct fld_name : ident {};
struct fld_data : p::must<fld_column, sep, fld_name, sep, fld_type, fld_flags> {
};

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

}  // parser
}  // spec
}  // core
}  // jumanpp

#endif  // JUMANPP_SPEC_FIELD_PARSER_H

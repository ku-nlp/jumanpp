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
struct bar : p::one<'|'> {};
struct lparen : p::one<'('> {};
struct rparen : p::one<')'> {};
struct eq : p::one<'='> {};
struct add : lit_string("+=") {};
struct colon : p::one<':'> {};

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

// seq for the internal level action to be called before this action
struct fldref_litem : p::seq<fieldparam> {};
struct fldref_list
    : p::if_must<lbrak, sep, p::list<fldref_litem, comma, sep_pad>, sep,
                 rbrak> {};
struct mt_lhs : p::sor<fldref_litem, fldref_list> {};
struct mt_rhs_file : p::seq<lit_string("file"), sep, qstring_param> {};
struct mt_rhs : p::sor<qstring_param, mt_rhs_file> {};
struct mt_cond : p::seq<mt_lhs, sep, lit_string("with"), sep, mt_rhs> {};
struct mt_then_body : p::seq<lit_string("then"), sep, fldref_list> {};
struct mt_else_body : p::seq<lit_string("else"), sep, fldref_list> {};
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
struct fref_lists : p::rep_min_max<1, 3, fref_list, sep> {};
struct ngram : p::if_must<lit_string("ngram"), sep, fref_lists> {};

struct char_type_lit : ident {};
struct char_type_expr;
struct char_type_or : p::seq<char_type_lit, sep, bar, sep, char_type_expr> {};
struct char_type_expr : p::sor<char_type_or, p::disable<char_type_lit>> {};

struct unk_ft_out : p::if_must<lit_string("feature to"), sep, fref_item> {};
struct unk_srf_out : p::if_must<lit_string("surface to"), sep, fldref_list> {};

struct unk_cls_single : p::if_must<lit_string("single"), sep, char_type_expr> {
};
struct unk_cls_chunking
    : p::if_must<lit_string("chunking"), sep, char_type_expr> {};
struct unk_cls_onoma
    : p::if_must<lit_string("onomatopeia"), sep, char_type_expr> {};
struct unk_cls_numeric
    : p::if_must<lit_string("numeric"), sep, char_type_expr> {};
struct unk_cls_normalize : p::if_must<lit_string("normalize")> {};
struct unk_cls : p::sor<unk_cls_single, unk_cls_chunking, unk_cls_onoma,
                        unk_cls_numeric, unk_cls_normalize> {};

struct unk_hdr : p::seq<strparam, sep, lit_string("template row"), sep,
                        snumparam, opt<sep, p::one<':'>>> {};
struct unk_flags : p::star<p::sor<unk_ft_out, unk_srf_out>, sep> {};
struct unk_def : p::if_must<lit_string("unk"), sep, unk_hdr, sep, unk_cls, sep,
                            unk_flags> {};

struct floatconst : p::seq<p::opt<p::one<'-', '+'>>, p::plus<p::digit>,
                           p::opt<p::one<'.'>, p::plus<p::digit>>> {};

struct train_field
    : p::seq<fieldparam, p::pad_opt<colon, sep_pad>, floatconst> {};
struct train_fields : p::list<train_field, comma, sep_pad> {};
struct train_gold_unk
    : p::if_must<lit_string("unk_gold_if"), sep, fldref_litem, sep, lbrak, sep,
                 qstring_param, sep, rbrak, sep, eq, eq, sep, fldref_litem> {};

struct train_stmt
    : p::if_must<lit_string("train"), p::opt<sep, lit_string("loss")>, sep,
                 train_fields, p::star<sep, train_gold_unk>> {};

struct spec_stmt
    : p::sor<fld_stmt, feature_stmt, unk_def, train_stmt, sep_pad> {};

struct full_spec : p::seq<p::star<spec_stmt, p::discard>, p::eof> {};

#undef lit_string

}  // namespace parser
}  // namespace spec
}  // namespace core
}  // namespace jumanpp

#endif  // JUMANPP_SPEC_FIELD_PARSER_H

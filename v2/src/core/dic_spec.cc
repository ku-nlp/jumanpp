//
// Created by Arseny Tolmachev on 2017/02/19.
//

#include "dic_spec.h"

#include <cstdlib>

#include <util/flatset.h>
#include <pegtl/ascii.hh>
#include <pegtl/input.hh>
#include <pegtl/parse.hh>
#include <pegtl/rules.hh>
#include <pegtl/trace.hh>

namespace jumanpp {
namespace core {
namespace dic {

namespace parser {

struct comment_content : pegtl::until<pegtl::at<pegtl::eol>> {};
struct line_comment : pegtl::seq<pegtl::one<'#'>, comment_content> {};

struct whitespace : pegtl::plus<pegtl::blank> {};
struct opt_whitespace : pegtl::opt<whitespace> {};

struct field_number : pegtl::plus<pegtl::digit> {};
struct field_name : pegtl::plus<pegtl::not_one<' ', '\t'>> {};
struct field_type_string : pegtl::istring<'s', 't', 'r', 'i', 'n', 'g'> {};
struct field_type_int : pegtl::istring<'i', 'n', 't'> {};
struct field_type_string_list
    : pegtl::istring<'s', 't', 'r', 'i', 'n', 'g', '_', 'l', 'i', 's', 't'> {};

struct field_type
    : pegtl::sor<field_type_string_list, field_type_string, field_type_int> {};

struct field_flag_trie_index
    : pegtl::istring<'t', 'r', 'i', 'e', '_', 'i', 'n', 'd', 'e', 'x'> {};

struct field_flag : pegtl::sor<field_flag_trie_index> {};
struct field_flags : pegtl::list<field_flag, opt_whitespace> {};
struct opt_flags : pegtl::opt<pegtl::seq<opt_whitespace, field_flags>> {};

struct descriptror : pegtl::seq<field_number, whitespace, field_name,
                                whitespace, field_type, opt_flags> {};

struct opt_descriptor : pegtl::opt<descriptror> {};

struct opt_line_comment : pegtl::opt<pegtl::seq<line_comment>> {};

struct line_content
    : pegtl::seq<opt_descriptor, opt_whitespace, opt_line_comment> {};
struct line : pegtl::must<opt_whitespace, line_content, pegtl::eolf> {};
struct descriptor_file : pegtl::until<pegtl::eof, line> {};

struct ParserState {
  ColumnDescriptor single;
  std::vector<ColumnDescriptor> collected;

  void reset() {
    single.position = 0;
    single.name = "";
    single.isTrieKey = false;
    single.columnType = ColumnType::Error;
    single.separatorRegex = " ";
  }
};

template <typename T>
struct parser_action {
  static void apply(const pegtl::input& in, ParserState& state) {}
};

template <>
struct parser_action<field_number> {
  static void apply(const pegtl::input& in, ParserState& state) {
    const auto endptr = in.end();
    state.single.position = static_cast<i32>(
        std::strtol(in.begin(), const_cast<char**>(&endptr), 10));
  }
};

template <>
struct parser_action<field_name> {
  static void apply(const pegtl::input& in, ParserState& state) {
    state.single.name.assign(in.begin(), in.end());
  }
};

template <>
struct parser_action<field_type_string> {
  static void apply(const pegtl::input& in, ParserState& state) {
    state.single.columnType = ColumnType::String;
  }
};

template <>
struct parser_action<field_type_int> {
  static void apply(const pegtl::input& in, ParserState& state) {
    state.single.columnType = ColumnType::Int;
  }
};

template <>
struct parser_action<field_type_string_list> {
  static void apply(const pegtl::input& in, ParserState& state) {
    state.single.columnType = ColumnType::StringList;
  }
};

template <>
struct parser_action<field_flag_trie_index> {
  static void apply(const pegtl::input& in, ParserState& state) {
    state.single.isTrieKey = true;
  }
};

template <>
struct parser_action<descriptror> {
  static void apply(const pegtl::input& in, ParserState& state) {
    state.collected.emplace_back(std::move(state.single));
    state.reset();
  }
};

}  // parser

Status findIndexColumn(const parser::ParserState& state, i32* result) {
  i32 count = 0;
  i32 column = -1;

  for (i32 i = 0; i < state.collected.size(); ++i) {
    auto& desc = state.collected[i];
    if (desc.isTrieKey) {
      count += 1;
      column = i;
    }
  }

  if (count == 0) {
    return Status::InvalidParameter() << "there was no column to be indexed";
  }

  if (count > 1) {
    return Status::InvalidParameter() << "only one column can be indexed, "
                                      << count << " found";
  }

  *result = column;
  return Status::Ok();
}

struct string_ref_hasher {
  std::hash<std::string> h;
  auto operator()(const std::string* value) const noexcept {
    const std::string& ref = *value;
    auto hv = h(ref);
    return hv;
  }
};

struct string_ref_eq {
  std::equal_to<std::string> eq;
  auto operator()(const std::string* s1, const std::string* s2) const noexcept {
    using ref_t = const std::string&;
    ref_t r1 = *s1;
    ref_t r2 = *s2;
    return eq(r1, r2);
  }
};

Status checkUniqueNames(const parser::ParserState& state) {
  jumanpp::util::FlatSet<const std::string*, string_ref_hasher, string_ref_eq>
      names;

  for (auto& descr : state.collected) {
    const std::string* ptr = &descr.name;
    if (names.count(ptr) != 0) {
      return Status::InvalidParameter()
             << "name " << descr.name << " was used at least for two columns. "
             << "Column names must be unique";
    }
    names.emplace(ptr);
  }

  return Status::Ok();
}

Status parseDescriptor(StringPiece data, DictionarySpec* result) {
  pegtl::input inp{0,  // line
                   0,  // column
                   data.begin(), data.end(), "descr"};

  parser::ParserState state;
  state.reset();

  bool success = false;

  try {
    success =
        pegtl::parse_input<parser::descriptor_file, parser::parser_action>(
            inp, state);
  } catch (pegtl::parse_error& err) {
    auto bldr = Status::InvalidParameter();
    bldr << "spec syntax error";
    for (auto& o : err.positions) {
      bldr << "\n     on line " << o.line << ":" << o.column << " content: "
           << StringPiece{o.begin, std::min(o.begin + 10, data.end())};
    }
    return bldr;
  }

  if (!success) {
    return Status::InvalidParameter() << "syntax error at: " << inp.line()
                                      << ":" << inp.column();
  }

  JPP_RETURN_IF_ERROR(findIndexColumn(state, &result->indexColumn));
  JPP_RETURN_IF_ERROR(checkUniqueNames(state));

  result->columns = std::move(state.collected);

  return Status::Ok();
}

}  // dic
}  // core
}  // jumanpp

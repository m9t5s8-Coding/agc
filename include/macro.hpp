#pragma once

#include "Statements/body.hpp"
namespace ag {

#define AG_EXPECT_OR_RETURN(parser_ref, tok, msg, fail_value)                                                          \
  do {                                                                                                                 \
    if (!(parser_ref).expect(tok, msg))                                                                                \
      return fail_value;                                                                                               \
  } while (0)

#define AG_CHECK_OR_RETURN(parser_ref, tok, fail_value)                                                                \
  do {                                                                                                                 \
    if (!(parser_ref).check(tok))                                                                                      \
      return fail_value;                                                                                               \
  } while (0)

#define AG_MATCH_OR_RETURN(parser_ref, tok, fail_value)                                                                \
  do {                                                                                                                 \
    if (!(parser_ref).match(tok))                                                                                      \
      return fail_value;                                                                                               \
  } while (0)

#define AG_EXPECT_ONE_OF_OR_RETURN(parser_ref, msg, out_matched, fail_value, ...)                                      \
  do {                                                                                                                 \
    if (!(parser_ref).expect_one_of({__VA_ARGS__}, msg, out_matched))                                                  \
      return fail_value;                                                                                               \
  } while (0)

#define AG_CHECK_ONE_OF_OR_RETURN(parser_ref, msg, out_matched, fail_value, ...)                                       \
  do {                                                                                                                 \
    if (!(parser_ref).check_one_of({__VA_ARGS__}, out_matched)) {                                                      \
      print_error(msg, parser_ref);                                                                                    \
      return fail_value;                                                                                               \
    }                                                                                                                  \
  } while (0)

#define AG_EXPECT_GET_VALUE_OR_RETURN(parser_ref, msg, out_var, fail_value, ...)                                       \
  do {                                                                                                                 \
    if (!(parser_ref).check_one_of({__VA_ARGS__})) {                                                                   \
      print_error(msg, parser_ref);                                                                                    \
      return fail_value;                                                                                               \
    }                                                                                                                  \
    (out_var).datatype_name = (parser_ref).current_token().value;                                                      \
    (out_var).datatype_type = get_datatypes((out_var).datatype_name);                                                  \
    (parser_ref).advance();                                                                                            \
  } while (0)

#define AG_EXPECT_GET_ID_OR_RETURN(parser_ref, msg, out_var, fail_value, tok)                                          \
  do {                                                                                                                 \
    if (!(parser_ref).check(tok)) {                                                                                    \
      print_error(msg, parser_ref);                                                                                    \
      return fail_value;                                                                                               \
    }                                                                                                                  \
    (out_var) = (parser_ref).current_token().value;                                                                    \
    (parser_ref).advance();                                                                                            \
  } while (0)

#define AG_PARSE_EXPRESSION_OR_RETURN(parser_ref, out_var, fail_value)                                                 \
  do {                                                                                                                 \
    auto _ag_parse_result = Expression::parse_expressions((parser_ref));                                               \
    if (!_ag_parse_result) {                                                                                           \
      print_error("Invalid!", (parser_ref));                                                                           \
      return fail_value;                                                                                               \
    }                                                                                                                  \
    (out_var) = std::move(_ag_parse_result);                                                                           \
  } while (0)

#define AG_PARSE_OPT_OR_RETURN(parser_ref, parser_fn, out_var, fail_value)                                             \
  do {                                                                                                                 \
    auto _ag_parse_result = parser_fn(parser_ref);                                                                     \
    if (!_ag_parse_result)                                                                                             \
      return fail_value;                                                                                               \
    (out_var) = std::move(*_ag_parse_result);                                                                          \
  } while (0)

#define AG_PARSE_STATEMENT(parser_ref, out_val, fail_value)                                                            \
  do {                                                                                                                 \
    auto _ag_parse_result = (parser_ref).parse_statement();                                                            \
    if (!_ag_parse_result)                                                                                             \
      return fail_value;                                                                                               \
    (out_val) = std::move(_ag_parse_result);                                                                           \
  } while (0)

#define AG_PARSE_SCOPE(parser_ref, out_val, fail_value)                                                                \
  do {                                                                                                                 \
    auto _ag_parse_result = BodyStatement::ParseBlock((parser_ref));                                                   \
    if (!_ag_parse_result)                                                                                             \
      return fail_value;                                                                                               \
    (out_val) = std::move(_ag_parse_result);                                                                           \
  } while (0)

} // namespace ag

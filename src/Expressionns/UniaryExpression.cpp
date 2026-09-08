#include "Expressions/UniaryExpression.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include <initializer_list>

namespace ag {
static UN_OPERATOR
get_prefix_operator(TokenName token) {
  switch (token) {
  case TokenName::AG_INCREMENT:
    return UN_OPERATOR::PRE_INCREMENT;

  case TokenName::AG_DECREMENT:
    return UN_OPERATOR::PRE_DECREMENT;

  case TokenName::AG_BITWISE_NOT:
    return UN_OPERATOR::BITWISE_NOT;

  case TokenName::AG_LOGICAL_NOT:
    return UN_OPERATOR::LOGICAl_NOT;

  case TokenName::AG_ADDRESS:
    return UN_OPERATOR::REFRENCE;

  case TokenName::AG_ASTRIC:
    return UN_OPERATOR::DEREFRENCE;

  case TokenName::AG_PLUS:
    return UN_OPERATOR::POSITIVE;

  case TokenName::AG_MINUS:
    return UN_OPERATOR::NEGATIVE;

  default:
    return UN_OPERATOR::NONE;
  }

  return UN_OPERATOR::NONE;
}

static UN_OPERATOR
get_postfix_operator(TokenName token) {
  switch (token) {
  case TokenName::AG_INCREMENT:
    return UN_OPERATOR::POST_INCREMENT;

  case TokenName::AG_DECREMENT:
    return UN_OPERATOR::POST_DECREMENT;

  default:
    return UN_OPERATOR::NONE;
  }
  return UN_OPERATOR::NONE;
}

AG_scope<Expression>
UniaryExpression::parse_prefix(Parser& parser) {
  static const std::initializer_list<TokenName> uniary_list = {
      TokenName::AG_INCREMENT,
      TokenName::AG_DECREMENT,
      TokenName::AG_BITWISE_NOT,
      TokenName::AG_LOGICAL_NOT,
      TokenName::AG_ADDRESS,
      TokenName::AG_ASTRIC,
      TokenName::AG_PLUS,
      TokenName::AG_MINUS,
  };
  TokenName temp_token;
  if (!parser.match_one_of(uniary_list, &temp_token)) {
    return nullptr;
  }
  auto prefix_expr         = AG_cscope<UniaryExpression>();
  prefix_expr->un_operator = get_prefix_operator(temp_token);
  prefix_expr->operand     = Expression::parse_expressions(parser);
  if (!prefix_expr->operand) {
    return nullptr;
  }
  return prefix_expr;
}

AG_scope<Expression>
UniaryExpression::parse_postfix(Parser&              parser,
                                AG_scope<Expression> operand) {
  static const std::initializer_list<TokenName> uniary_list = {
      TokenName::AG_INCREMENT,
      TokenName::AG_DECREMENT,
  };
  TokenName temp_token;
  if (!parser.match_one_of(uniary_list, &temp_token)) {
    return nullptr;
  }
  auto postfix_expr         = AG_cscope<UniaryExpression>();
  postfix_expr->un_operator = get_postfix_operator(temp_token);
  postfix_expr->operand     = std::move(operand);

  return postfix_expr;
}
} // namespace ag

#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
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
  case TokenName::AG_REFERENCE:
    return UN_OPERATOR::REFRENCE;
  case TokenName::AG_ASTERISK:
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

static BIN_OPERATOR
get_binary_operator(TokenName token) {
  switch (token) {
  case TokenName::AG_ASSIGN:
    return BIN_OPERATOR::ASSIGN_TO;
  case TokenName::AG_LOGICAL_OR:
    return BIN_OPERATOR::LOGICAL_OR;
  case TokenName::AG_LOGICAL_AND:
    return BIN_OPERATOR::LOGICAL_AND;
  case TokenName::AG_BITWISE_OR:
    return BIN_OPERATOR::BITWISE_OR;
  case TokenName::AG_BITWISE_XOR:
    return BIN_OPERATOR::BITWISE_XOR;
  case TokenName::AG_REFERENCE:
    return BIN_OPERATOR::BITWISE_AND;
  case TokenName::AG_EQUAL_TO:
    return BIN_OPERATOR::EQUAL_TO;
  case TokenName::AG_NOT_EQUAL_TO:
    return BIN_OPERATOR::NOT_EQUAL_TO;
  case TokenName::AG_LESS:
    return BIN_OPERATOR::LESS_THAN;
  case TokenName::AG_GREATER:
    return BIN_OPERATOR::GREATER_THAN;
  case TokenName::AG_LESS_EQUAL:
    return BIN_OPERATOR::LESS_THAN_EQUAL;
  case TokenName::AG_GREATER_EQUAL:
    return BIN_OPERATOR::GREATER_THAN_EQUAL;
  case TokenName::AG_LEFT_SHIFT:
    return BIN_OPERATOR::LEFT_SHIFT;
  case TokenName::AG_RIGHT_SHIFT:
    return BIN_OPERATOR::RIGHT_SHIFT;
  case TokenName::AG_PLUS:
    return BIN_OPERATOR::ADDITION;
  case TokenName::AG_MINUS:
    return BIN_OPERATOR::SUBTRACTION;
  case TokenName::AG_ASTERISK:
    return BIN_OPERATOR::MULTIPLICATION;
  case TokenName::AG_SLASH:
    return BIN_OPERATOR::DIVISION;
  case TokenName::AG_MODULO:
    return BIN_OPERATOR::MODULUS;
  default:
    return BIN_OPERATOR::NONE;
  }
}

static constexpr int POSTFIX_BP = 100;

// static int
// led_binding_power(TokenName token) {
//   switch (token) {
//   case TokenName::AG_DOT:
//   case TokenName::AG_ARROW:
//   case TokenName::AG_LEFT_PAREN:
//   case TokenName::AG_LEFT_BRACE:
//   case TokenName::AG_INCREMENT:
//   case TokenName::AG_DECREMENT:
//     return POSTFIX_BP;
//   default:
//     break;
//   }

static std::tuple<float,
                  float>
get_binding_power(BIN_OPERATOR bin_op) {

  switch (bin_op) {
  case BIN_OPERATOR::ASSIGN_TO:
    return {1.0f, 1.1f};
  case BIN_OPERATOR::LOGICAL_OR:
    return {2.0f, 2.1f};
  case BIN_OPERATOR::LOGICAL_AND:
    return {3.0f, 3.1f};
  case BIN_OPERATOR::BITWISE_OR:
    return {4.0f, 4.1f};
  case BIN_OPERATOR::BITWISE_XOR:
    return {5.0f, 5.1f};
  case BIN_OPERATOR::BITWISE_AND:
    return {6.0f, 6.1f};
  case BIN_OPERATOR::EQUAL_TO:
  case BIN_OPERATOR::NOT_EQUAL_TO:
    return {7.0f, 7.1f};
  case BIN_OPERATOR::LESS_THAN:
  case BIN_OPERATOR::GREATER_THAN:
  case BIN_OPERATOR::LESS_THAN_EQUAL:
  case BIN_OPERATOR::GREATER_THAN_EQUAL:
    return {8.0f, 8.1f};
  case BIN_OPERATOR::LEFT_SHIFT:
  case BIN_OPERATOR::RIGHT_SHIFT:
    return {9.0f, 9.1f};
  case BIN_OPERATOR::ADDITION:
  case BIN_OPERATOR::SUBTRACTION:
    return {10.0f, 10.1f};
  case BIN_OPERATOR::MULTIPLICATION:
  case BIN_OPERATOR::DIVISION:
  case BIN_OPERATOR::MODULUS:
    return {11.0f, 11.1f};
  default:
    return {0.0f, 0.1f};
  }
}

static const std::initializer_list<TokenName> binary_list = {
    TokenName::AG_ASSIGN,      TokenName::AG_LOGICAL_OR,  TokenName::AG_LOGICAL_AND, TokenName::AG_BITWISE_OR,
    TokenName::AG_BITWISE_XOR, TokenName::AG_REFERENCE,   TokenName::AG_EQUAL_TO,    TokenName::AG_NOT_EQUAL_TO,
    TokenName::AG_LESS,        TokenName::AG_GREATER,     TokenName::AG_LESS_EQUAL,  TokenName::AG_GREATER_EQUAL,
    TokenName::AG_LEFT_SHIFT,  TokenName::AG_RIGHT_SHIFT, TokenName::AG_PLUS,        TokenName::AG_MINUS,
    TokenName::AG_ASTERISK,    TokenName::AG_SLASH,       TokenName::AG_MODULO,
};

static bool
is_right_associative(BIN_OPERATOR op) {
  return op == BIN_OPERATOR::ASSIGN_TO;
}

AG_scope<Expression>
Expression::parse_expressions(Parser& parser) {
  return parse_pratt(parser, 0.0f);
}

AG_scope<Expression>
Expression::parse_pratt(Parser& parser,
                        float   min_bp) {
  AG_scope<Expression> left;
  if (parser.match(TokenName::AG_LEFT_PAREN)) {
    left = parse_pratt(parser, 0.0f);
    if (!left)
      return nullptr;
    if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen)) {
      return nullptr;
    }
  } else {
    left = UniaryExpression::parse_prefix(parser);
    if (!left) {
      return nullptr;
    }
  }

  TokenName temp_token;
  while (true) {
    if (!parser.check_one_of(binary_list, &temp_token)) {
      break;
    }
    BIN_OPERATOR bin_op = get_binary_operator(temp_token);
    auto [l_bp, r_bp]   = get_binding_power(bin_op);
    if (l_bp < min_bp) {
      break;
    }
    parser.advance();
    auto right = parse_pratt(parser, r_bp);
    if (!right) {
      return nullptr;
    }
    left = AG_cscope<BinaryExpression>(bin_op, std::move(left), std::move(right));
  }
  return left;
}

AG_scope<Expression>
PrimaryExpression::parse_primary(Parser& parser) {
  TokenName temp_token;
  if (!parser.check_one_of({TokenName::AG_STRING_LITERAL, TokenName::AG_INT_LITERAL, TokenName::AG_IDENTIFIER},
                           &temp_token)) {
    return nullptr;
  }
  auto primary_exp = AG_cscope<PrimaryExpression>();
  if (temp_token == TokenName::AG_STRING_LITERAL) {
    primary_exp->primary_type = PRIMARY_TYPE::STRING_LITERAL;
    primary_exp->value        = parser.current_token().value;
  } else if (temp_token == TokenName::AG_INT_LITERAL) {
    primary_exp->primary_type = PRIMARY_TYPE::INT_LITERAL;
    primary_exp->value        = parser.current_token().value;
  } else if (temp_token == TokenName::AG_IDENTIFIER) {
    primary_exp->primary_type = PRIMARY_TYPE::IDENTIFIER;
    primary_exp->value        = parser.current_token().value;
  }
  parser.advance();
  return primary_exp;
}

AG_scope<Expression>
MemberExpression::parse(Parser&              parser,
                        AG_scope<Expression> object) {
  TokenName op_tok;
  parser.peek_token_name(&op_tok);
  parser.advance(); // consumes '.' or '->'

  if (!parser.expect(TokenName::AG_IDENTIFIER, "")) {
    return nullptr;
  }

  auto member        = AG_cscope<MemberExpression>();
  member->object     = std::move(object);
  member->field_name = parser.current_token().value;
  member->is_arrow   = (op_tok == TokenName::AG_ARROW);
  return member;
}

AG_scope<Expression>
CallExpression::parse(Parser&              parser,
                      AG_scope<Expression> callee) {
  parser.advance(); // consume '('

  auto call    = AG_cscope<CallExpression>();
  call->callee = std::move(callee);

  if (!parser.check(TokenName::AG_RIGHT_PAREN)) {
    do {
      auto arg = Expression::parse_pratt(parser, 0);
      if (!arg) {
        return nullptr;
      }
      call->arguments.push_back(std::move(arg));
    } while (parser.match(TokenName::AG_COMMA));
  }

  if (!parser.expect(TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen)) {
    return nullptr;
  }
  return call;
}

AG_scope<Expression>
IndexExpression::parse(Parser&              parser,
                       AG_scope<Expression> object) {
  parser.advance(); // consume '['

  auto idx_expr    = AG_cscope<IndexExpression>();
  idx_expr->object = std::move(object);
  idx_expr->index  = Expression::parse_pratt(parser, 0);
  if (!idx_expr->index) {
    return nullptr;
  }

  if (!parser.expect(TokenName::AG_RIGHT_BRACKET, error::ExpectedRightBracket)) {
    return nullptr;
  }
  return idx_expr;
}

AG_scope<Expression>
UniaryExpression::parse_prefix(Parser& parser) {
  static const std::initializer_list<TokenName> uniary_list = {
      TokenName::AG_INCREMENT,
      TokenName::AG_DECREMENT,
      TokenName::AG_BITWISE_NOT,
      TokenName::AG_LOGICAL_NOT,
      TokenName::AG_REFERENCE,
      TokenName::AG_ASTERISK,
      TokenName::AG_PLUS,
      TokenName::AG_MINUS,
  };
  TokenName temp_token;
  if (parser.check_one_of(uniary_list, &temp_token)) {
    auto prefix_expr         = AG_cscope<UniaryExpression>();
    prefix_expr->un_operator = get_prefix_operator(temp_token);
    parser.advance();
    prefix_expr->operand = Expression::parse_pratt(parser, POSTFIX_BP - 1);
    if (!prefix_expr->operand) {
      return nullptr;
    }
    return prefix_expr;
  }

  return PrimaryExpression::parse_primary(parser);
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

#include "Expressions/PrimaryExpression.hpp"
#include "Parser.hpp"

namespace ag {
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
}; // namespace ag

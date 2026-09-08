#pragma once
#include "Expressions/Expression.hpp"

namespace ag {
class PrimaryExpression : public Expression {
public:
  static AG_scope<Expression>
  parse_primary(Parser& parser);

private:
  std::string_view value;
  enum class PRIMARY_TYPE { INT_LITERAL, STRING_LITERAL, IDENTIFIER, NONE } primary_type = PRIMARY_TYPE::NONE;
};
} // namespace ag

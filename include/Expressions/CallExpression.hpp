#pragma once
#include "Expressions/Expression.hpp"

namespace ag {
class Parser;
class CallExpression : public Expression {
public:
  static AG_scope<Expression>
  parse_callexpression(Parser&);

private:
};
} // namespace ag

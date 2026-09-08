#pragma once

#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"

namespace ag {

enum class BN_OPERATOR {
  ADDITION,
  SUBTRACTION,
  MULTIPLICATION,
  DIVISION,
  MODULUS,
  BITWISE_XOR,
  BITWISE_AND,
  BITWISE_OR,
  LESS_THAN,
  LESS_THAN_EQUAL,
  GREATER_THAN,
  GREATER_THAN_EQUAL,
  EQUAL_TO,
  ASSIGN_TO,
  NOT_EQUAL_TO,
  LOGICAL_AND,
  LOGICAL_OR,
  LEFT_SHIFT,
  RIGHT_SHIFT,
  INDEXING,
  ARROW
};

class BinaryExpression : public Expression {
public:
  BinaryExpression() {}
  virtual ~BinaryExpression() {}

  static AG_scope<Expression>
  parse_binary(Parser&);

private:
  AG_scope<Expression> left;
  AG_scope<Expression> right;
  BN_OPERATOR          bn_operator;
};
} // namespace ag

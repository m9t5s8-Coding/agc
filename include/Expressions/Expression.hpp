#pragma once
#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression {
public:
  Expression() {}
  virtual ~Expression() {}

  static AG_scope<Expression>
  parse_expressions(Parser& parser);
};
} // namespace ag

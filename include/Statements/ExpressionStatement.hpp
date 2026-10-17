#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression;
class ExpressionStatement : public Statements {
public:
  static AG_scope<Statements>
  parse_expression_statement(Parser&);

  virtual void
  generate(CodeGenContext&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_expression;
};
} // namespace ag

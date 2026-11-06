#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression;
class ExpressionStatement : public Statements {
public:
  static AG_scope<Statements>
  parse_expression_statement(Parser&);

  // Wrap an expression parsed from `func f() => expr;` when the function has a
  // void return type.  The expression is evaluated for its side effects and no
  // value is returned.
  static AG_scope<Statements>
  Create(AG_scope<Expression> expression,
         SourceLocation      location);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_expression;
};
} // namespace ag

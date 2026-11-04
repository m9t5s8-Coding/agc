
#pragma once
#include "Statements/Statements.hpp"

namespace ag {
class Expression;
class Parser;
class ReturnStatement : public Statements {
public:
  virtual ~ReturnStatement() {}

  static AG_scope<Statements>
  ParseReturn(Parser& parser);

  // Wrap an expression parsed from `func f() -> T => expr;` in a return
  // statement so expression-bodied functions share the normal return path.
  static AG_scope<Statements>
  Create(AG_scope<Expression> expression,
         SourceLocation      location);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_return_expr;
};
} // namespace ag

#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression;

// guard (condition) else {
//   ...
// }
//
// The else body runs when the condition is false; otherwise execution
// continues after the guard.  This is the statement form; guard expressions
// with pattern bindings are not implemented yet.
class GuardStatement : public Statements {
public:
  GuardStatement() {}
  virtual ~GuardStatement() {}

  static AG_scope<Statements>
  ParseGuard(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_condition;
  AG_scope<Statements> m_else_body;
};
} // namespace ag

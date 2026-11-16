#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;

// `defer statement;` runs its body when the enclosing block ends.  Deferred
// statements run in reverse order at normal block exit; cleanup on early
// `return`/`break` is not implemented yet.
class DeferStatement : public Statements {
public:
  DeferStatement() {}
  virtual ~DeferStatement() {}

  static AG_scope<Statements>
  ParseDefer(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Statements> m_body;
};
} // namespace ag

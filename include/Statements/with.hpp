#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;

// `with (let file = open(path)) { ... }` declares one or more bindings and
// runs the body in an inner scope.  Automatic resource cleanup (the RAII part
// of `with`) is not implemented yet.
class WithStatement : public Statements {
public:
  WithStatement() {}
  virtual ~WithStatement() {}

  static AG_scope<Statements>
  ParseWith(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  std::vector<AG_scope<Statements>> m_declarations;
  AG_scope<Statements>              m_body;
};
} // namespace ag

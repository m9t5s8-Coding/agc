#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;

  // `loop { ... }` repeats its body until a `break` executes.  It may also be
  // used as an expression; `break value` supplies the expression result.
class LoopStatement : public Statements {
public:
  LoopStatement() {}
  virtual ~LoopStatement() {}

  static AG_scope<Statements>
  ParseLoop(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

  virtual void
  set_loop_label(std::string label) override {
    m_label = std::move(label);
  }

private:
  AG_scope<Statements> m_body;
  std::string          m_label;
};
} // namespace ag

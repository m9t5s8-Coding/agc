#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Expression;
class Parser;

// `break;` exits the innermost loop.  `break value;` and labelled breaks are
// accepted syntactically, but loop expressions and labels are not implemented
// yet.
class BreakStatement : public Statements {
public:
  BreakStatement() {}
  virtual ~BreakStatement() {}

  static AG_scope<Statements>
  ParseBreak(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  std::string          m_label;
  AG_scope<Expression> m_value;
};
} // namespace ag

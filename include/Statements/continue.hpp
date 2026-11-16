#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Expression;
class Parser;

// `continue;` jumps to the next iteration of the innermost loop.  Labelled
// continues are accepted syntactically but labels are not implemented yet.
class ContinueStatement : public Statements {
public:
  ContinueStatement() {}
  virtual ~ContinueStatement() {}

  static AG_scope<Statements>
  ParseContinue(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  std::string m_label;
};
} // namespace ag

#pragma once
#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression;
class WhileStatement : public Statements {
public:
  WhileStatement() {}
  virtual ~WhileStatement() {}

  static AG_scope<Statements>
  ParseWhile(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_condition;
  AG_scope<Statements> m_while_body;
};
} // namespace ag

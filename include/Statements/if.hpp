#pragma once

#include "Statements/Statements.hpp"
namespace ag {
class Parser;
class Expression;
class IfStatement : public Statements {
public:
  IfStatement() {}
  virtual ~IfStatement() {}

  static AG_scope<Statements>
  ParseIf(Parser& parser);

  virtual void
  generate(CodeGenContext&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_condition;
  AG_scope<Statements> m_if_body;
  AG_scope<Statements> m_else_body;
};
} // namespace ag

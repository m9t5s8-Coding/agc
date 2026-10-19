#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Expression;
class Parser;
class ExitStatement : public Statements {
public:
  virtual ~ExitStatement() {}

  static AG_scope<Statements>
  ParseExit(Parser& parser);

  virtual void
  generate_ir(CodeGenContext&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_exit_expr;
};
} // namespace ag

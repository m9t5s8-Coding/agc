#pragma once
#include "Statements/Statements.hpp"

namespace ag {
class Expression;
class Parser;
class ExitStatement : public Statements {
public:
  virtual ~ExitStatement() {}

  static AG_scope<Statements>
  ParseExit(Parser& parser);

  virtual void
  generate(CodeGenContext&) override;

private:
  AG_scope<Expression> exit_expr;
};
} // namespace ag

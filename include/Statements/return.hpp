
#pragma once
#include "Statements/Statements.hpp"

namespace ag {
class Expression;
class Parser;
class ReturnStatement : public Statements {
public:
  virtual ~ReturnStatement() {}

  static AG_scope<Statements>
  ParseReturn(Parser& parser);

  virtual void
  generate_ir(CodeGenContext&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_return_expr;
};
} // namespace ag

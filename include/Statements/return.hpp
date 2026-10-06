
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
  generate(CodeGenContext&) override;

  virtual bool
  analyze_semantics(Parser& parser) override {
    return true;
  }

private:
  AG_scope<Expression> return_expr;
};
} // namespace ag

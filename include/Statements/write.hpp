#pragma once
#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression;
class WriteStatement : public Statements {
public:
  virtual ~WriteStatement() {}

  static AG_scope<Statements>
  ParseWrite(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

private:
  AG_scope<Expression> m_buffer;
  AG_scope<Expression> m_buffer_size;
};
} // namespace ag

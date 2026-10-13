#pragma once
#include "Statements/Statements.hpp"

namespace ag {
class Expression;
class Parser;
class ReadStatement : public Statements {
public:
  virtual ~ReadStatement() {}

  static AG_scope<Statements>
  ParseRead(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Expression> m_buffer;
  AG_scope<Expression> m_buffer_size;
};
} // namespace ag

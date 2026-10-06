#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Expression;
class Parser;
class LetStatement : public Statements {
public:
  virtual ~LetStatement() {}

  static AG_scope<Statements>
  ParseLet(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  DataType             m_datatype;
  std::string_view     m_name;
  AG_scope<Expression> m_expr;
  SourceLocation       m_location;
};
} // namespace ag

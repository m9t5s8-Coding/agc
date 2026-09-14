
#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Expression;
class Parser;
class VarStatement : public Statements {
public:
  virtual ~VarStatement() {}

  static AG_scope<Statements>
  ParseVar(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

private:
  DataType_Data        m_datatype;
  std::string_view     m_name;
  AG_scope<Expression> m_expr;
};
} // namespace ag

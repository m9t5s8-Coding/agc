
#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class ReturnStatement : public Statements {
public:
  ReturnStatement() : value_or_name("") {}
  virtual ~ReturnStatement() {}

  static AG_scope<Statements>
  ParseReturn(Parser& parser);

  virtual void
  generate(CodeGenContext&) override;

private:
  std::string value_or_name;
  // AG_scope<Expression> return_expression;
};
} // namespace ag

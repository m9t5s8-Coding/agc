#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class ExitStatement : public Statements {
public:
  ExitStatement() : value_or_name("") {}
  virtual ~ExitStatement() {}

  static AG_scope<Statements>
  ParseExit(Parser& parser);

  virtual void
  generate(CodeGenContext&) override;

private:
  std::string value_or_name;
  EXPR_TYPE   type;
};
} // namespace ag

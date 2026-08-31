#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class LetStatement : public Statements {
public:
  LetStatement() : variable_name("") {}
  LetStatement(const std::string& name) : variable_name(name) {}

  virtual ~LetStatement() {}

  static AG_scope<Statements>
  ParseLet(Parser& parser);

  virtual void
  generate(CodeGenContext& context) override;

private:
  std::string variable_name;

  static AG_scope<Statements>
  LetString(Parser&,
            size_t);
};
} // namespace ag

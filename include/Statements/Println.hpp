#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class PrintlnStatement : public Statements {
public:
  PrintlnStatement()
    : buffer("") {}
  PrintlnStatement(const std::string& data)
    : buffer(data) {}

  virtual ~PrintlnStatement() {}

  static AG_scope<Statements> ParsePrintln(Parser& parser, TokenName token);

  virtual void generate(CodeGenContext& context) override;

private:
  std::string                 buffer;
  static AG_scope<Statements> PrintString(Parser&);
  static AG_scope<Statements> PrintVariable(Parser&);
};
}  // namespace ag

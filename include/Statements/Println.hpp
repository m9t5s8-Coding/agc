#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class PrintlnStatement : public Statements {
public:
  PrintlnStatement()
    : buffer(""),
      is_literal{true} {}
  PrintlnStatement(const std::string& data, bool is_literal)
    : buffer(data),
      is_literal(is_literal) {}
  PrintlnStatement(const std::string& data)
    : buffer(data),
      is_literal(false) {}

  virtual ~PrintlnStatement() {}

  static AG_scope<Statements> ParsePrintln(Parser& parser, TokenName token);

  virtual void generate(CodeGenContext& context) override;

private:
  std::string                 buffer;
  bool                        is_literal = true;
  static AG_scope<Statements> PrintString(Parser&);
  static AG_scope<Statements> PrintVariable(Parser&);
};
}  // namespace ag

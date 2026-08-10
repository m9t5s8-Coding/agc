#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"

namespace ag {

class ExitStatement : public Statements {
public:
  ExitStatement()
    : exit_code(1) {}
  ExitStatement(int exit_code)
    : exit_code(exit_code) {}

  virtual ~ExitStatement() {}

  static AG_scope<Statements> ParseExit(Parser& parser, TokenName token);

  virtual void generate(CodeGenContext&) override;

private:
  int                         exit_code;
  static AG_scope<Statements> ExitWithSmallBracket(Parser&);
  static AG_scope<Statements> ExitWithoutSmallBracket(Parser&);
};
}  // namespace ag

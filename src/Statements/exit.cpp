#include "Statements/exit.hpp"

#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"
#include <iostream>

namespace ag {

AG_scope<Statements>
ExitStatement::ParseExit(Parser& parser) {
  parser.advance();

  auto exit_stmt = AG_cscope<ExitStatement>();

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, exit_stmt->exit_expr, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return exit_stmt;
}

void
ExitStatement::generate(CodeGenContext& context) {}

bool
ExitStatement::analyze_semantics(Parser& parser) {
  auto primary_expr = dynamic_cast<PrimaryExpression*>(exit_expr.get());
  if (primary_expr) {
    if (primary_expr->get_type() == PRIMARY_TYPE::INT_LITERAL) {
      return true;
    }
  }
  return false;
}

} // namespace ag

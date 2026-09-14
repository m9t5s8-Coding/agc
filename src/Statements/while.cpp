#include "Statements/while.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
WhileStatement::ParseWhile(Parser& parser) {
  parser.advance();

  auto while_stmt = AG_cscope<WhileStatement>();

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, while_stmt->m_condition, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, while_stmt->m_while_body, nullptr);

  return while_stmt;
}

void
WhileStatement::generate(CodeGenContext&) {}
} // namespace ag

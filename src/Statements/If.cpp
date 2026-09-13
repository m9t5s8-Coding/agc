#include "Statements/If.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
IfStatement::ParseIf(Parser& parser) {
  parser.advance();

  auto if_stmt = AG_cscope<IfStatement>();

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, if_stmt->m_condition, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, if_stmt->m_if_body, nullptr);
  AG_MATCH_OR_RETURN(parser, TokenName::AG_ELSE, if_stmt);
  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, if_stmt->m_else_body, nullptr);

  return if_stmt;
}
void
IfStatement::generate(CodeGenContext&) {}

} // namespace ag

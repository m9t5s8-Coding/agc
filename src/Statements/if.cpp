#include "Statements/if.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
IfStatement::ParseIf(Parser& parser) {
  parser.advance();

  auto if_stmt             = AG_cscope<IfStatement>();
  if_stmt->m_location.line = parser.current_token().line;

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

bool
IfStatement::analyze_semantics(Parser& parser) {
  if (!m_condition->analyze_semantics(parser, m_location)) {
    return false;
  }
  auto return_val = m_if_body->analyze_semantics(parser);
  if (!m_else_body)
    return return_val;
  return return_val && m_else_body->analyze_semantics(parser);
}

} // namespace ag

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

  auto while_stmt             = AG_cscope<WhileStatement>();
  while_stmt->m_location.line = parser.current_token().line;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, while_stmt->m_condition, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, while_stmt->m_while_body, nullptr);

  return while_stmt;
}
// while(x < 10)
void
WhileStatement::generate_ir(CodeGenContext& context,
                            Parser&         parser) {
  auto cond_l = context.block_label++;
  auto then_l = context.block_label++;
  auto end_l  = context.block_label++;
  // context.text << 'b' << cond_l << ":\n";
  auto cond = m_condition->generate_ir(context, parser);

  // context.text << "if " << cond << " jump b" << then_l << " else jump b" << end_l << "\nb" << then_l << ":\n";
  m_while_body->generate_ir(context, parser);

  // context.text << "jump b" << cond_l << '\n' << "b" << end_l << ":\n";
}

bool
WhileStatement::analyze_semantics(Parser& parser) {
  if (!m_condition->analyze_semantics(parser, m_location)) {
    return false;
  }
  return m_while_body->analyze_semantics(parser);
}

} // namespace ag

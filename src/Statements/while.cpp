#include "Statements/while.hpp"
#include "Expressions/Expression.hpp"
#include "IR/IR.hpp"
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
void
WhileStatement::generate_ir(LabelCounters& counter,
                            Parser&         parser) {
  auto cond_l = counter.block_label++;
  auto then_l = counter.block_label++;
  auto end_l  = counter.block_label++;

  auto str = 'b' + std::to_string(cond_l);
  parser.add_instructios(AG_cscope<Label>(str));
  auto cond = m_condition->generate_ir(counter, parser);

  str = 'b' + std::to_string(end_l);
  parser.add_instructios(AG_cscope<CondJump>(cond, str));
  str = 'b' + std::to_string(then_l);
  parser.add_instructios(AG_cscope<Label>(str));
  m_while_body->generate_ir(counter, parser);

  str = 'b' + std::to_string(cond_l);
  parser.add_instructios(AG_cscope<Jump>(str));
  str = 'b' + std::to_string(end_l);
  parser.add_instructios(AG_cscope<Label>(str));
}

bool
WhileStatement::analyze_semantics(Parser& parser) {
  if (!m_condition->analyze_semantics(parser, m_location)) {
    return false;
  }
  return m_while_body->analyze_semantics(parser);
}

} // namespace ag

#include "Statements/guard.hpp"

#include "Expressions/Expression.hpp"
#include "IR/CondJump.hpp"
#include "IR/IR.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
GuardStatement::ParseGuard(Parser& parser) {
  const Token statement_token = parser.current_token();
  parser.advance(); // `guard`

  auto guard_stmt        = AG_cscope<GuardStatement>();
  guard_stmt->m_location = {statement_token.line, statement_token.column};

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, guard_stmt->m_condition, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
  AG_MATCH_OR_RETURN(parser, TokenName::AG_ELSE, guard_stmt);
  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, guard_stmt->m_else_body, nullptr);

  return guard_stmt;
}

void
GuardStatement::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  const auto condition = m_condition->generate_ir(counter, parser);
  const auto else_l    = counter.block_label++;
  const auto end_l     = counter.block_label++;

  // A true condition skips the else body; a false condition enters it.
  const auto else_label = "b" + std::to_string(else_l);
  parser.add_instructios(
      AG_cscope<CondJump>(condition, else_label, counter.function_index));
  parser.add_instructios(AG_cscope<Jump>("b" + std::to_string(end_l)));

  parser.add_instructios(AG_cscope<Label>(else_label));
  m_else_body->generate_ir(counter, parser);

  parser.add_instructios(AG_cscope<Label>("b" + std::to_string(end_l)));
}

bool
GuardStatement::analyze_semantics(Parser& parser) {
  if (!m_condition->analyze_semantics(parser, m_condition->source_location(m_location)))
    return false;
  return m_else_body->analyze_semantics(parser);
}
} // namespace ag

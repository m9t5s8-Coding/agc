#include "Statements/while.hpp"
#include "Expressions/Expression.hpp"
#include "IR/IR.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
WhileStatement::ParseWhile(Parser& parser) {
  return parse_loop(parser, false);
}

AG_scope<Statements>
WhileStatement::ParseUntil(Parser& parser) {
  return parse_loop(parser, true);
}

AG_scope<Statements>
WhileStatement::parse_loop(Parser& parser,
                           bool    until) {
  const Token statement_token = parser.current_token();
  parser.advance();

  auto while_stmt        = AG_cscope<WhileStatement>();
  while_stmt->m_location = {statement_token.line, statement_token.column};
  while_stmt->m_until    = until;

  if (parser.match(TokenName::AG_LET)) {
    while_stmt->m_is_let = true;
    const SourceLocation pattern_location = {
        parser.current_token().line, parser.current_token().column};
    if (!parse_match_pattern(parser, while_stmt->m_pattern, pattern_location))
      return nullptr;
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_ASSIGN, error::ExpectedEqual, nullptr);
    AG_PARSE_EXPRESSION_OR_RETURN(parser, while_stmt->m_condition, nullptr);
  } else {
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
    AG_PARSE_EXPRESSION_OR_RETURN(parser, while_stmt->m_condition, nullptr);
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
  }

  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, while_stmt->m_while_body, nullptr);

  return while_stmt;
}

void
WhileStatement::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  auto cond_l = counter.block_label++;
  auto then_l = counter.block_label++;
  auto end_l  = counter.block_label++;

  const std::string cond_label = "b" + std::to_string(cond_l);

  parser.add_instructios(AG_cscope<Label>(cond_label));

  std::string condition;
  std::string scrutinee;
  if (m_is_let) {
    scrutinee = m_condition->generate_ir(counter, parser);
    condition =
        generate_match_pattern_condition(counter, parser, m_pattern, scrutinee);
  } else {
    condition = m_condition->generate_ir(counter, parser);

    // `until (cond)` is the complement of `while (cond)`: leave the loop when
    // the condition becomes true.
    if (m_until) {
      const auto inverted = make_temp(counter, parser);
      parser.add_instructios(AG_cscope<Binary>(inverted,
                                               condition,
                                               "1",
                                               BIN_OPERATOR::BITWISE_XOR,
                                               counter.function_index));
      condition = inverted;
    }
  }

  if (!condition.empty())
    parser.add_instructios(AG_cscope<CondJump>(
        condition, "b" + std::to_string(end_l), counter.function_index));

  parser.add_instructios(AG_cscope<Label>("b" + std::to_string(then_l)));
  if (m_is_let)
    generate_match_pattern_bindings(counter, parser, m_pattern, scrutinee);

  parser.push_loop_labels("b" + std::to_string(end_l), cond_label, m_label);
  m_while_body->generate_ir(counter, parser);
  parser.pop_loop_labels();

  parser.add_instructios(AG_cscope<Jump>(cond_label));
  parser.add_instructios(AG_cscope<Label>("b" + std::to_string(end_l)));
}

bool
WhileStatement::analyze_semantics(Parser& parser) {
  if (!m_condition->analyze_semantics(parser,
                                      m_condition->source_location(m_location))) {
    return false;
  }

  if (m_is_let)
    get_symbol_table().enter_scope();

  bool pattern_ok = true;
  if (m_is_let) {
    pattern_ok = analyze_match_pattern(parser,
                                       m_pattern,
                                       m_condition->get_data_type(),
                                       m_pattern.location);
  }

  parser.enter_loop();
  const bool body_ok = pattern_ok && m_while_body->analyze_semantics(parser);
  parser.exit_loop();

  if (m_is_let)
    get_symbol_table().exit_scope();

  return body_ok;
}

} // namespace ag

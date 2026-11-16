#include "Statements/if.hpp"
#include "Expressions/Expression.hpp"
#include "IR/CondJump.hpp"
#include "IR/IR.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
IfStatement::ParseIf(Parser& parser) {
  const Token statement_token = parser.current_token();
  parser.advance();

  auto if_stmt        = AG_cscope<IfStatement>();
  if_stmt->m_location = {statement_token.line, statement_token.column};

  if (parser.match(TokenName::AG_LET)) {
    // `if let pattern = expression { ... }`
    if_stmt->m_is_let = true;
    const SourceLocation pattern_location = {
        parser.current_token().line, parser.current_token().column};
    if (!parse_match_pattern(parser, if_stmt->m_pattern, pattern_location))
      return nullptr;
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_ASSIGN, error::ExpectedEqual, nullptr);
    AG_PARSE_EXPRESSION_OR_RETURN(parser, if_stmt->m_condition, nullptr);
  } else {
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
    AG_PARSE_EXPRESSION_OR_RETURN(parser, if_stmt->m_condition, nullptr);
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);
  }

  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, if_stmt->m_if_body, nullptr);
  AG_MATCH_OR_RETURN(parser, TokenName::AG_ELSE, if_stmt);

  // `else if` is represented as a nested IfStatement in the else slot.
  if (parser.check(TokenName::AG_IF)) {
    if_stmt->m_else_body = ParseIf(parser);
    if (!if_stmt->m_else_body)
      return nullptr;
  } else {
    AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
    AG_PARSE_SCOPE(parser, if_stmt->m_else_body, nullptr);
  }

  return if_stmt;
}
void
IfStatement::generate_ir(LabelCounters& counter,
                         Parser&        parser) {
  const auto then_l  = counter.block_label++;
  const auto else_l  = m_else_body ? counter.block_label++ : 0;
  const auto end_l   = counter.block_label++;
  const auto false_l = m_else_body ? else_l : end_l;

  if (m_is_let) {
    const auto scrutinee = m_condition->generate_ir(counter, parser);
    const auto condition =
        generate_match_pattern_condition(counter, parser, m_pattern, scrutinee);
    if (!condition.empty())
      parser.add_instructios(AG_cscope<CondJump>(
          condition, "b" + std::to_string(false_l), counter.function_index));

    parser.add_instructios(AG_cscope<Label>("b" + std::to_string(then_l)));
    generate_match_pattern_bindings(counter, parser, m_pattern, scrutinee);
  } else {
    const auto cond = m_condition->generate_ir(counter, parser);
    parser.add_instructios(AG_cscope<CondJump>(
        cond, "b" + std::to_string(false_l), counter.function_index));

    parser.add_instructios(AG_cscope<Label>("b" + std::to_string(then_l)));
  }

  m_if_body->generate_ir(counter, parser);

  parser.add_instructios(
      AG_cscope<Jump>("b" + std::to_string(end_l), counter.function_index));

  if (m_else_body) {
    parser.add_instructios(AG_cscope<Label>("b" + std::to_string(else_l)));
    m_else_body->generate_ir(counter, parser);
    parser.add_instructios(
        AG_cscope<Jump>("b" + std::to_string(end_l), counter.function_index));
  }

  parser.add_instructios(AG_cscope<Label>("b" + std::to_string(end_l)));
}

bool
IfStatement::analyze_semantics(Parser& parser) {
  if (!m_condition->analyze_semantics(parser,
                                      m_condition->source_location(m_location))) {
    return false;
  }

  if (m_is_let)
    get_symbol_table().enter_scope();

  bool ok = true;
  if (m_is_let) {
    ok = analyze_match_pattern(parser,
                               m_pattern,
                               m_condition->get_data_type(),
                               m_pattern.location);
  }
  if (ok)
    ok = m_if_body->analyze_semantics(parser);

  if (m_is_let)
    get_symbol_table().exit_scope();

  if (!ok)
    return false;

  if (!m_else_body)
    return true;
  return m_else_body->analyze_semantics(parser);
}

} // namespace ag

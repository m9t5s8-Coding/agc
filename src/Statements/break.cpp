#include "Statements/break.hpp"

#include "Expressions/Expression.hpp"
#include "IR/Assign.hpp"
#include "IR/IR.hpp"
#include "IR/Jump.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
BreakStatement::ParseBreak(Parser& parser) {
  const Token break_token = parser.current_token();
  parser.advance(); // `break`

  auto break_stmt        = AG_cscope<BreakStatement>();
  break_stmt->m_location = {break_token.line, break_token.column};

  // `break label;` is only a labelled break when the name matches an
  // enclosing loop; otherwise it is `break value;`.
  if (parser.check(TokenName::AG_IDENTIFIER) &&
      parser.is_loop_label_name(parser.current_token().value)) {
    break_stmt->m_label = parser.current_token().value;
    parser.advance();
    if (!parser.match(TokenName::AG_SEMICOLON)) {
      AG_PARSE_EXPRESSION_OR_RETURN(parser, break_stmt->m_value, nullptr);
      AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);
    }
    return break_stmt;
  }

  if (!parser.match(TokenName::AG_SEMICOLON)) {
    AG_PARSE_EXPRESSION_OR_RETURN(parser, break_stmt->m_value, nullptr);
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);
  }

  return break_stmt;
}

void
BreakStatement::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  const auto* labels = parser.find_loop_labels(m_label);
  if (!labels)
    return;

  if (m_value && !labels->result_slot.empty()) {
    const auto value = m_value->generate_ir(counter, parser);
    parser.add_instructios(
        AG_cscope<Assign>(value, labels->result_slot, counter.function_index));
  }

  parser.add_instructios(AG_cscope<Jump>(labels->break_label));
}

bool
BreakStatement::analyze_semantics(Parser& parser) {
  if (!parser.inside_loop()) {
    print_error(error::UnexpectedToken, parser, m_location);
    return false;
  }

  if (m_value &&
      !m_value->analyze_semantics(parser, m_value->source_location(m_location)))
    return false;

  return true;
}
} // namespace ag

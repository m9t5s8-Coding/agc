#include "Statements/continue.hpp"

#include "Expressions/Expression.hpp"
#include "IR/IR.hpp"
#include "IR/Jump.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
ContinueStatement::ParseContinue(Parser& parser) {
  const Token continue_token = parser.current_token();
  parser.advance(); // `continue`

  auto continue_stmt        = AG_cscope<ContinueStatement>();
  continue_stmt->m_location = {continue_token.line, continue_token.column};

  if (!parser.match(TokenName::AG_SEMICOLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(error::ExpectedIdentifier, parser);
      return nullptr;
    }
    continue_stmt->m_label = parser.current_token().value;
    parser.advance();
    AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);
  }

  return continue_stmt;
}

void
ContinueStatement::generate_ir(LabelCounters& counter,
                               Parser&        parser) {
  (void)counter;
  if (const auto* labels = parser.find_loop_labels(m_label))
    parser.add_instructios(AG_cscope<Jump>(labels->continue_label));
}

bool
ContinueStatement::analyze_semantics(Parser& parser) {
  if (!parser.inside_loop()) {
    print_error(error::UnexpectedToken, parser, m_location);
    return false;
  }
  return true;
}
} // namespace ag

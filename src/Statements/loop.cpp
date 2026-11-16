#include "Statements/loop.hpp"

#include "IR/IR.hpp"
#include "IR/Jump.hpp"
#include "IR/Label.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
LoopStatement::ParseLoop(Parser& parser) {
  const Token loop_token = parser.current_token();
  parser.advance(); // `loop`

  auto loop_stmt        = AG_cscope<LoopStatement>();
  loop_stmt->m_location = {loop_token.line, loop_token.column};

  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  AG_PARSE_SCOPE(parser, loop_stmt->m_body, nullptr);

  return loop_stmt;
}

void
LoopStatement::generate_ir(LabelCounters& counter,
                           Parser&        parser) {
  const auto body_label = "b" + std::to_string(counter.block_label++);
  const auto end_label  = "b" + std::to_string(counter.block_label++);

  parser.add_instructios(AG_cscope<Label>(body_label));
  parser.push_loop_labels(end_label, body_label, m_label);
  m_body->generate_ir(counter, parser);
  parser.pop_loop_labels();
  parser.add_instructios(AG_cscope<Jump>(body_label));
  parser.add_instructios(AG_cscope<Label>(end_label));
}

bool
LoopStatement::analyze_semantics(Parser& parser) {
  parser.enter_loop();
  const bool body_ok = m_body->analyze_semantics(parser);
  parser.exit_loop();
  return body_ok;
}
} // namespace ag

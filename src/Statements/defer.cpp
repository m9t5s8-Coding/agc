#include "Statements/defer.hpp"

#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
DeferStatement::ParseDefer(Parser& parser) {
  const Token defer_token = parser.current_token();
  parser.advance(); // `defer`

  auto defer_stmt        = AG_cscope<DeferStatement>();
  defer_stmt->m_location = {defer_token.line, defer_token.column};

  defer_stmt->m_body = parser.parse_statement();
  if (!defer_stmt->m_body)
    return nullptr;

  return defer_stmt;
}

void
DeferStatement::generate_ir(LabelCounters& counter,
                            Parser&        parser) {
  // BodyStatement skips deferred statements in source order and calls this at
  // block exit, in reverse order.
  if (m_body)
    m_body->generate_ir(counter, parser);
}

bool
DeferStatement::analyze_semantics(Parser& parser) {
  return m_body && m_body->analyze_semantics(parser);
}
} // namespace ag

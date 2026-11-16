#include "Statements/test.hpp"

#include "Parser.hpp"
#include "Statements/body.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
TestStatement::ParseTest(Parser& parser) {
  const Token test_token = parser.current_token();
  parser.advance(); // `test`

  auto test_stmt        = AG_cscope<TestStatement>();
  test_stmt->m_location = {test_token.line, test_token.column};

  if (parser.check(TokenName::AG_STRING_LITERAL))
    parser.advance();

  if (!parser.check(TokenName::AG_LEFT_BRACE)) {
    print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedLeftBrace,
                parser);
    return nullptr;
  }

  test_stmt->m_body = BodyStatement::ParseBlock(parser);
  if (!test_stmt->m_body)
    return nullptr;

  return test_stmt;
}

void
TestStatement::generate_ir(LabelCounters&,
                           Parser&) {
  // Tests are compiled out of normal executables.
}

bool
TestStatement::analyze_semantics(Parser&) {
  // The body referenced test-only helpers such as `assert`; do not resolve
  // names until a test harness exists.
  return true;
}
} // namespace ag

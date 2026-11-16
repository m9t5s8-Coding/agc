#include "Statements/with.hpp"

#include "Parser.hpp"
#include "Statements/body.hpp"
#include "Statements/Statements.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
WithStatement::ParseWith(Parser& parser) {
  const Token with_token = parser.current_token();
  parser.advance(); // `with`

  auto with_stmt        = AG_cscope<WithStatement>();
  with_stmt->m_location = {with_token.line, with_token.column};

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);

  while (!parser.check(TokenName::AG_RIGHT_PAREN)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    if (!parser.check_one_of({TokenName::AG_LET, TokenName::AG_VAR,
                              TokenName::AG_CONST})) {
      print_error(error::UnexpectedToken, parser);
      return nullptr;
    }

    auto declaration = parser.parse_statement();
    if (!declaration)
      return nullptr;
    with_stmt->m_declarations.emplace_back(std::move(declaration));

    if (!parser.match(TokenName::AG_COMMA))
      break;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, error::ExpectedRightParen, nullptr);

  AG_CHECK_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, nullptr);
  with_stmt->m_body = BodyStatement::ParseBlock(parser);
  if (!with_stmt->m_body)
    return nullptr;

  return with_stmt;
}

void
WithStatement::generate_ir(LabelCounters& counter,
                           Parser&        parser) {
  for (const auto& declaration : m_declarations)
    declaration->generate_ir(counter, parser);
  m_body->generate_ir(counter, parser);
}

bool
WithStatement::analyze_semantics(Parser& parser) {
  get_symbol_table().enter_scope();

  for (const auto& declaration : m_declarations) {
    if (!declaration->analyze_semantics(parser)) {
      get_symbol_table().exit_scope();
      return false;
    }
  }

  const bool body_ok = m_body->analyze_semantics(parser);
  get_symbol_table().exit_scope();
  return body_ok;
}
} // namespace ag

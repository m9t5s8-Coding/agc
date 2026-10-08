#include "Statements/exit.hpp"

#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {

AG_scope<Statements>
ExitStatement::ParseExit(Parser& parser) {
  parser.advance();

  auto exit_stmt             = AG_cscope<ExitStatement>();
  exit_stmt->m_location.line = parser.current_token().line;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, exit_stmt->m_exit_expr, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return exit_stmt;
}

void
ExitStatement::generate(CodeGenContext& context) {}

bool
ExitStatement::analyze_semantics(Parser& parser) {
  auto type = m_exit_expr->get_type();
  if (type == PrimitiveType::UNEXPECTED) {
    print_error(error::UseOfUndefined, parser, m_location);
    return false;
  }
  if (!is_compatible_datatype(PrimitiveType::INT, type)) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }
  return true;
}

} // namespace ag

#include "Statements/ExpressionStatement.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"

namespace ag {
AG_scope<Statements>
ExpressionStatement::parse_expression_statement(Parser& parser) {
  auto expr_statement = AG_cscope<ExpressionStatement>();
  auto expr           = Expression::parse_expressions(parser);
  if (!expr)
    return nullptr;
  if (!parser.expect(TokenName::AG_SEMICOLON, error::ExpectedSemicolon)) {
    return nullptr;
  }
  expr_statement->m_expression = std::move(expr);
  return expr_statement;
}
} // namespace ag

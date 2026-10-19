#include "Statements/ExpressionStatement.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"

namespace ag {
AG_scope<Statements>
ExpressionStatement::parse_expression_statement(Parser& parser) {
  auto expr_statement             = AG_cscope<ExpressionStatement>();
  expr_statement->m_location.line = parser.current_token().line;
  auto expr                       = Expression::parse_expressions(parser);
  if (!expr) {
    return nullptr;
  }
  expr_statement->m_expression = std::move(expr);
  if (!parser.expect(TokenName::AG_SEMICOLON, error::ExpectedSemicolon)) {
    return nullptr;
  }
  return expr_statement;
}

bool
ExpressionStatement::analyze_semantics(Parser& parser) {
  return m_expression->analyze_semantics(parser, m_location);
}

void
ExpressionStatement::generate_ir(CodeGenContext& context,
                                 Parser&         parser) {
  m_expression->generate_ir(context, parser);
}
} // namespace ag

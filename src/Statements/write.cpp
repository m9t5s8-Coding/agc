#include "Statements/write.hpp"

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {

// use expression and later use expression in that
AG_scope<Statements>
WriteStatement::ParseWrite(Parser& parser) {
  parser.advance();

  auto write_statement             = AG_cscope<WriteStatement>();
  write_statement->m_location.line = parser.current_token().line;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, ag::error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, write_statement->m_buffer, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COMMA, ag::error::ExpectedComma, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, write_statement->m_buffer_size, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return write_statement;
}

void
WriteStatement::generate(CodeGenContext& context) {
  {
    auto exp = m_buffer->generate((context));
    if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_buffer.get());
        bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
      exp = bin_expr->left->generate(context);
    }
    context.text << "param " << exp << '\n';
  }
  {
    auto exp = m_buffer_size->generate((context));
    if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_buffer_size.get());
        bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
      exp = bin_expr->left->generate(context);
    }
    context.text << "param " << exp << '\n';
  }

  context.text << "call __ag_write()\n";
}

bool
WriteStatement::analyze_semantics(Parser& parser) {
  if (!m_buffer->analyze_semantics(parser, m_location)) {
    return false;
  }
  auto type = m_buffer->get_type();
  if (!is_compatible_datatype(PrimitiveType::STRING, type)) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }

  if (!m_buffer_size->analyze_semantics(parser, m_location)) {
    return false;
  }
  type = m_buffer_size->get_type();
  if (!is_compatible_datatype(PrimitiveType::INT, type)) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }

  return true;
}
} // namespace ag

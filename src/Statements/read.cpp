#include "Statements/read.hpp"

#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "IR/IR.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {

// use expression and later use expression in that
AG_scope<Statements>
ReadStatement::ParseRead(Parser& parser) {
  parser.advance();

  auto read_statement             = AG_cscope<ReadStatement>();
  read_statement->m_location.line = parser.current_token().line;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, ag::error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, read_statement->m_buffer, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COMMA, ag::error::ExpectedComma, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, read_statement->m_buffer_size, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return read_statement;
}

bool
ReadStatement::analyze_semantics(Parser& parser) {
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

void
ReadStatement::generate_ir(CodeGenContext& context,
                           Parser&         parser) {
  {
    auto exp = m_buffer->generate_ir(context, parser);
    if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_buffer.get());
        bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
      exp = bin_expr->left->generate_ir(context, parser);
    }
    parser.add_instructios(AG_cscope<Param>(exp));
  }
  {
    auto exp = m_buffer_size->generate_ir(context, parser);
    if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_buffer_size.get());
        bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
      exp = bin_expr->left->generate_ir(context, parser);
    }
    parser.add_instructios(AG_cscope<Param>(exp));
  }
  parser.add_instructios(AG_cscope<FunctionCall>("__ag_read"));
}

} // namespace ag

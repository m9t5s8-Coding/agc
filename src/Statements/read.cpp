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
  const Token statement_token = parser.current_token();
  parser.advance();

  auto read_statement = AG_cscope<ReadStatement>();
  read_statement->m_location = {statement_token.line, statement_token.column};

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
  const auto& buffer_location = m_buffer->source_location(m_location);
  if (!m_buffer->analyze_semantics(parser, buffer_location)) {
    return false;
  }
  DataType expected;
  expected.primitive_type = PrimitiveType::STRING;
  expected.type_name      = "string";
  if (!is_compatible_type(expected, m_buffer->get_data_type())) {
    print_error(error::IncompatibleTypes, parser, buffer_location);
    return false;
  }

  const auto& size_location = m_buffer_size->source_location(m_location);
  if (!m_buffer_size->analyze_semantics(parser, size_location)) {
    return false;
  }
  if (!is_compatible_datatype(PrimitiveType::INT, m_buffer_size->get_type())) {
    print_error(error::IncompatibleTypes, parser, size_location);
    return false;
  }
  return true;
}

void
ReadStatement::generate_ir(LabelCounters& counter,
                           Parser&        parser) {
  // Evaluate both arguments before loading either argument register.  A call
  // inside one argument may clobber registers already loaded for the other.
  auto buffer_exp = m_buffer->generate_ir(counter, parser);
  if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_buffer.get());
      bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    buffer_exp = bin_expr->left->generate_ir(counter, parser);
  }

  auto size_exp = m_buffer_size->generate_ir(counter, parser);
  if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_buffer_size.get());
      bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    size_exp = bin_expr->left->generate_ir(counter, parser);
  }

  parser.add_instructios(AG_cscope<Param>(buffer_exp, counter.function_index));
  parser.add_instructios(AG_cscope<Param>(size_exp, counter.function_index));
  parser.add_instructios(AG_cscope<FunctionCall>("__ag_read"));
}

} // namespace ag

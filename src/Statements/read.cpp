#include "Statements/read.hpp"

#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
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
  auto type = m_buffer->get_type();
  if (type == PrimitiveType::UNEXPECTED) {
    print_error(error::UseOfUndefined, parser, m_location);
    return false;
  }
  if (!is_compatible_datatype(PrimitiveType::STRING, type)) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }

  type = m_buffer_size->get_type();
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

void
ReadStatement::generate(CodeGenContext& context) {

  context.text << "lea rsi, [" << m_buffer << "]\n"
               << "mov rdx, " << m_buffer_size << "\n"
               << "call __ag_read\n";
}

} // namespace ag

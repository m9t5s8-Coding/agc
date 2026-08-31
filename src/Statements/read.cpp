#include "Statements/read.hpp"

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
  auto write_statement = AG_cscope<ReadStatement>();

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, ag::error::ExpectedLeftParen, nullptr);
  AG_EXPECT_GET_VALUE_OR_RETURN(
      parser, TokenName::AG_IDENTIFIER, ag::error::ExpectedIdentifier, write_statement->m_buffer, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COMMA, ag::error::ExpectedComma, nullptr);
  AG_EXPECT_GET_VALUE_OR_RETURN(
      parser, TokenName::AG_IDENTIFIER, ag::error::ExpectedIdentifier, write_statement->m_buffer_size, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return write_statement;
}

void
ReadStatement::generate(CodeGenContext& context) {

  context.text << "lea rsi, [" << m_buffer << "]\n"
               << "mov rdx, " << m_buffer_size << "\n"
               << "call __ag_read\n";
}

} // namespace ag

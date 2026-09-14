#include "Statements/var.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
VarStatement::ParseVar(Parser& parser) {
  parser.advance();

  auto var_stmt = AG_cscope<VarStatement>();

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedIdentifier, var_stmt->m_name, nullptr, TokenName::AG_IDENTIFIER);

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, nullptr);

  AG_EXPECT_GET_VALUE_OR_RETURN(parser,
                                ag::error::ExpectedKeyword,
                                var_stmt->m_datatype,
                                nullptr,
                                TokenName::AG_KEYWORD,
                                TokenName::AG_IDENTIFIER);

  if (parser.match(TokenName::AG_ASSIGN)) {
    AG_PARSE_EXPRESSION_OR_RETURN(parser, var_stmt->m_expr, nullptr);
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);

  return var_stmt;
}

void
VarStatement::generate(CodeGenContext& context) {}
} // namespace ag

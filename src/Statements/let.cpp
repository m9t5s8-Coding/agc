#include "Statements/let.hpp"
#include "Expressions/Expression.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "SymbolTable.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
LetStatement::ParseLet(Parser& parser) {
  parser.advance();

  auto let_stmt             = AG_cscope<LetStatement>();
  let_stmt->m_location.line = parser.current_token().line;

  AG_EXPECT_GET_ID_OR_RETURN(
      parser, ag::error::ExpectedIdentifier, let_stmt->m_name, nullptr, TokenName::AG_IDENTIFIER);

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_COLON, error::ExpectedColon, nullptr);

  AG_EXPECT_GET_VALUE_OR_RETURN(parser,
                                ag::error::ExpectedKeyword,
                                let_stmt->m_datatype,
                                nullptr,
                                TokenName::AG_KEYWORD,
                                TokenName::AG_IDENTIFIER);

  let_stmt->m_datatype.is_const = true;

  if (parser.match(TokenName::AG_ASSIGN)) {
    AG_PARSE_EXPRESSION_OR_RETURN(parser, let_stmt->m_expr, nullptr);
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, error::ExpectedSemicolon, nullptr);

  return let_stmt;
}

void
LetStatement::generate(CodeGenContext& context) {}

bool
LetStatement::analyze_semantics(Parser& parser) {
  // let x : int = 3;
  Declaration data;
  data.type     = m_datatype;
  data.timing   = ValueTiming::COMPILE_TIME;
  data.location = m_location;
  if (!get_symbol_table().insert(m_name, data)) {
    print_error(error::RedefinitionOfSymbol, parser, m_location);
    return false;
  }

  if (!m_expr)
    return true;
  auto type = m_expr->get_type();
  if (type == PrimitiveType::UNEXPECTED) {
    print_error(error::UseOfUndefined, parser, m_location);
    return false;
  }
  if (!is_compatible_datatype(m_datatype.primitive_type, m_expr->get_type())) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }

  return true;
}

} // namespace ag

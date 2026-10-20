#include "Statements/return.hpp"

#include "Expressions/Expression.hpp"
#include "IR/IR.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {

AG_scope<Statements>
ReturnStatement::ParseReturn(Parser& parser) {
  parser.advance();

  auto return_stmt             = AG_cscope<ReturnStatement>();
  return_stmt->m_location.line = parser.current_token().line;

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_PAREN, error::ExpectedLeftParen, nullptr);
  AG_PARSE_EXPRESSION_OR_RETURN(parser, return_stmt->m_return_expr, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_RIGHT_PAREN, ag::error::ExpectedRightParen, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return return_stmt;
}

void
ReturnStatement::generate_ir(CodeGenContext& context,
                             Parser&         parser) {
  auto exp = m_return_expr->generate_ir(context, parser);
  if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_return_expr.get());
      bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
    exp = bin_expr->left->generate_ir(context, parser);
  }
  // context.text << "param " << exp << '\n' << "return\n";
  parser.add_instructios(AG_cscope<Param>(exp));
  parser.add_instructios(AG_cscope<Return>());
}

bool
ReturnStatement::analyze_semantics(Parser& parser) {
  if (!m_return_expr->analyze_semantics(parser, m_location)) {
    return false;
  }
  auto type = m_return_expr->get_type();
  if (!is_compatible_datatype(PrimitiveType::INT, type)) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }
  return true;
}
} // namespace ag

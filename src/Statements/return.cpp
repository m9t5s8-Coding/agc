#include "Statements/return.hpp"

#include "Expressions/Expression.hpp"
#include "IR/IR.hpp"
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
// #include "SymbolTable.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {

AG_scope<Statements>
ReturnStatement::ParseReturn(Parser& parser) {
  const auto return_line = parser.current_token().line;
  parser.advance();

  auto return_stmt             = AG_cscope<ReturnStatement>();
  return_stmt->m_location.line = return_line;

  // `return;` is valid for void functions.  Otherwise the expression may be
  // written either with or without parentheses: `return x;` and
  // `return(x);` are both accepted.
  if (parser.match(TokenName::AG_SEMICOLON))
    return return_stmt;

  AG_PARSE_EXPRESSION_OR_RETURN(parser, return_stmt->m_return_expr, nullptr);
  AG_EXPECT_OR_RETURN(parser, TokenName::AG_SEMICOLON, ag::error::ExpectedSemicolon, nullptr);

  return return_stmt;
}

void
ReturnStatement::generate_ir(LabelCounters& counter,
                             Parser&        parser) {
  std::string exp;
  if (m_return_expr) {
    exp = m_return_expr->generate_ir(counter, parser);
    if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_return_expr.get());
        bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
      exp = bin_expr->left->generate_ir(counter, parser);
    }
  }
  parser.add_instructios(AG_cscope<Return>(exp, counter.function_index));
}

bool
ReturnStatement::analyze_semantics(Parser& parser) {
  const auto* return_type = parser.current_function_return_type();
  if (!return_type) {
    print_error(error::ReturnOutsideFunction, parser, m_location);
    return false;
  }

  if (!m_return_expr) {
    if (return_type->primitive_type != PrimitiveType::VOID) {
      print_error(error::MissingReturnValue, parser, m_location);
      return false;
    }
    return true;
  }

  if (!m_return_expr->analyze_semantics(parser, m_location))
    return false;

  if (return_type->primitive_type == PrimitiveType::VOID) {
    print_error(error::InvalidReturnType, parser, m_location);
    return false;
  }

  const auto expr_type = m_return_expr->get_type();
  if (expr_type == PrimitiveType::UNEXPECTED ||
      !is_compatible_datatype(return_type->primitive_type, expr_type)) {
    print_error(error::IncompatibleTypes, parser, m_location);
    return false;
  }

  return true;
}
} // namespace ag

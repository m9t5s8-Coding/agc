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
ReturnStatement::Create(AG_scope<Expression> expression,
                        SourceLocation      location) {
  auto return_stmt        = AG_cscope<ReturnStatement>();
  return_stmt->m_location = location;
  return_stmt->m_return_expr = std::move(expression);
  return return_stmt;
}

AG_scope<Statements>
ReturnStatement::ParseReturn(Parser& parser) {
  const Token return_token = parser.current_token();
  parser.advance();

  auto return_stmt       = AG_cscope<ReturnStatement>();
  return_stmt->m_location = {return_token.line, return_token.column};

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
  const auto* return_type = parser.current_function_return_type();
  bool        reference_return = false;
  if (m_return_expr) {
    reference_return =
        return_type && return_type->kind == TypeKind::REFERENCE &&
        m_return_expr->get_declared_data_type().kind == TypeKind::REFERENCE;
    exp = reference_return ? m_return_expr->generate_reference_ir(counter, parser)
                           : m_return_expr->generate_ir(counter, parser);
    if (auto bin_expr = dynamic_cast<BinaryExpression*>(m_return_expr.get());
        bin_expr && bin_expr->bin_operator == BIN_OPERATOR::ASSIGN_TO) {
      exp = bin_expr->left->generate_ir(counter, parser);
    }
    if (return_type && !reference_return)
      exp = narrow_value(exp, *return_type, counter, parser);
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

  if (!m_return_expr->analyze_semantics(parser, m_return_expr->source_location(m_location)))
    return false;

  if (return_type->primitive_type == PrimitiveType::VOID) {
    print_error(error::InvalidReturnType, parser, m_location);
    return false;
  }

  const auto expr_type   = m_return_expr->get_data_type();
  const auto expr_declared = m_return_expr->get_declared_data_type();
  const bool reference_return =
      return_type->kind == TypeKind::REFERENCE &&
      expr_declared.kind == TypeKind::REFERENCE &&
      is_compatible_type(*return_type, expr_declared);
  if (expr_type.primitive_type == PrimitiveType::UNEXPECTED ||
      (!is_compatible_type(*return_type, expr_type) && !reference_return)) {
    print_error(error::IncompatibleTypes, parser, m_return_expr->source_location(m_location));
    return false;
  }

  return true;
}
} // namespace ag

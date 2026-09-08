#include "Expressions/Expression.hpp"
// #include "Expressions/BinaryExpression.hpp"
#include "Expressions/PrimaryExpression.hpp"
#include "Expressions/UniaryExpression.hpp"
#include "Parser.hpp"

namespace ag {
// ++buffer  ++
AG_scope<Expression>
Expression::parse_expressions(Parser& parser) {
  auto expr = UniaryExpression::parse_prefix(parser);
  if (expr)
    return expr;
  expr = PrimaryExpression::parse_primary(parser);
  if (expr) {
    return expr;
  }
  return expr;
  UniaryExpression::parse_postfix(parser, std::move(expr));
  return nullptr;
}
} // namespace ag

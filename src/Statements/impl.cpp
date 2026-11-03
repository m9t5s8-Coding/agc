#include "Statements/impl.hpp"

#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/function.hpp"
#include "Statements/Utils.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
AG_scope<Statements>
ImplStatement::ParseImpl(Parser& parser) {
  const Token impl_token = parser.current_token();
  parser.advance(); // `impl`

  auto impl_stmt        = AG_cscope<ImplStatement>();
  impl_stmt->m_location = {impl_token.line, impl_token.column};

  std::string first_name;
  AG_EXPECT_GET_ID_OR_RETURN(
      parser, error::ExpectedTypeName, first_name, nullptr, TokenName::AG_IDENTIFIER);

  // Accept the trait form `impl Shape for Circle { ... }` by ignoring the
  // trait name.  Trait dispatch is not part of the current feature set.
  if (parser.check(TokenName::AG_IDENTIFIER) && parser.current_token().value == "for") {
    parser.advance();
    AG_EXPECT_GET_ID_OR_RETURN(
        parser, error::ExpectedTypeName, impl_stmt->m_type_name, nullptr, TokenName::AG_IDENTIFIER);
  } else {
    impl_stmt->m_type_name = std::move(first_name);
  }

  if (!get_struct_table().lookup(impl_stmt->m_type_name)) {
    print_error(error::UndefinedType, parser, impl_stmt->m_location);
    return nullptr;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace, nullptr);

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    if (!parser.check(TokenName::AG_FUNC)) {
      print_error(error::ExpectedStatement, parser);
      return nullptr;
    }

    parser.push_impl_type(impl_stmt->m_type_name);
    auto method = FunctionStatement::ParseFunction(parser);
    parser.pop_impl_type();

    if (!method)
      return nullptr;

    impl_stmt->m_methods.push_back(std::move(method));
  }

  parser.advance(); // `}`
  return impl_stmt;
}

void
ImplStatement::generate_ir(LabelCounters& counter,
                           Parser&        parser) {
  for (const auto& method : m_methods)
    method->generate_ir(counter, parser);
}

bool
ImplStatement::analyze_semantics(Parser& parser) {
  for (const auto& method : m_methods) {
    if (!method->analyze_semantics(parser))
      return false;
  }
  return true;
}
} // namespace ag

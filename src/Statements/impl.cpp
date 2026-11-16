#include "Statements/impl.hpp"

#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/function.hpp"
#include "Statements/Utils.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

namespace ag {
namespace {
std::optional<std::string>
parse_type_path(Parser& parser) {
  if (!parser.check(TokenName::AG_IDENTIFIER)) {
    print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedTypeName,
                parser);
    return std::nullopt;
  }

  std::string path(parser.current_token().value);
  parser.advance();

  while (parser.match(TokenName::AG_DOUBLE_COLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedTypeName,
                  parser);
      return std::nullopt;
    }
    path += "::";
    path += parser.current_token().value;
    parser.advance();
  }

  return parser.resolve_type_name(path);
}
} // namespace

AG_scope<Statements>
ImplStatement::ParseImpl(Parser& parser) {
  const Token impl_token = parser.current_token();
  parser.advance(); // `impl`
  parser.take_pending_public(); // `pub impl` has no additional effect yet

  auto impl_stmt        = AG_cscope<ImplStatement>();
  impl_stmt->m_location = {impl_token.line, impl_token.column};

  // `impl<T> Stack<T>` is treated as `impl Stack` until generic instantiation
  // exists.  The parameter names remain visible while methods are parsed.
  const bool has_generic_parameters = parser.check(TokenName::AG_LESS);
  if (has_generic_parameters) {
    auto generic_parameters = parse_generic_parameter_list(parser);
    if (!generic_parameters)
      return nullptr;
    parser.push_generic_parameters(std::move(*generic_parameters));
  }

  auto first_name = parse_type_path(parser);
  if (!first_name)
    return nullptr;
  if (parser.check(TokenName::AG_LESS) && !skip_generic_arguments(parser))
    return nullptr;

  // Accept the trait form `impl Shape for Circle { ... }` by ignoring the
  // trait name.  Trait dispatch is not part of the current feature set.
  if (parser.check(TokenName::AG_FOR)) {
    parser.advance();
    auto owner_name = parse_type_path(parser);
    if (!owner_name)
      return nullptr;
    if (parser.check(TokenName::AG_LESS) && !skip_generic_arguments(parser))
      return nullptr;
    impl_stmt->m_type_name = std::move(*owner_name);
  } else {
    impl_stmt->m_type_name = std::move(*first_name);
  }

  if (!get_struct_table().lookup(impl_stmt->m_type_name) &&
      !get_enum_table().lookup(impl_stmt->m_type_name)) {
    print_error(error::UndefinedType, parser, impl_stmt->m_location);
    return nullptr;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace, nullptr);

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    bool method_public = false;
    bool method_static = false;
    // Accept `pub`, `priv`, and `static` in any order before the method.
    while (true) {
      if (parser.match(TokenName::AG_PUB)) {
        method_public = true;
        continue;
      }
      if (parser.match(TokenName::AG_PRIV)) {
        method_public = false;
        continue;
      }
      if (parser.match(TokenName::AG_STATIC)) {
        method_static = true;
        continue;
      }
      break;
    }
    parser.set_pending_public(method_public);
    parser.set_pending_static(method_static);

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
  if (has_generic_parameters)
    parser.pop_generic_parameters();
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

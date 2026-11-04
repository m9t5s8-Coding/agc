#include "Statements/import.hpp"

#include "Parser.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include "error_msg.hpp"

#include <string>

namespace ag {
namespace {
std::string
last_path_segment(const std::string& path) {
  const auto separator = path.rfind("::");
  return separator == std::string::npos ? path : path.substr(separator + 2);
}

std::string
resolve_import_path(Parser& parser, const std::string& path) {
  const auto        separator = path.find("::");
  const std::string first     = path.substr(0, separator);
  const std::string& scope    = parser.current_module_path();
  const std::string suffix =
      separator == std::string::npos ? std::string{} : path.substr(separator);

  if (const auto* alias = get_module_table().find_module_alias(scope, first))
    return *alias + suffix;
  if (!scope.empty()) {
    if (const auto* alias = get_module_table().find_module_alias({}, first))
      return *alias + suffix;
  }

  if (!scope.empty()) {
    const std::string candidate = scope + "::" + path;
    if (get_module_table().has_module(candidate))
      return candidate;
  }

  return path;
}
} // namespace

AG_scope<Statements>
ImportStatement::ParseImport(Parser& parser) {
  const Token import_token = parser.current_token();
  parser.advance(); // `import`

  auto import_stmt        = AG_cscope<ImportStatement>();
  import_stmt->m_location = {import_token.line, import_token.column};

  if (!parser.check(TokenName::AG_IDENTIFIER)) {
    print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedImportPath,
                parser);
    return nullptr;
  }

  std::string path(parser.current_token().value);
  parser.advance();

  while (parser.match(TokenName::AG_DOUBLE_COLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedImportPath,
                  parser);
      return nullptr;
    }
    path += "::";
    path += parser.current_token().value;
    parser.advance();
  }

  const std::string& scope         = parser.current_module_path();
  const std::string  resolved_path = resolve_import_path(parser, path);

  if (parser.match(TokenName::AG_AS)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedAliasName,
                  parser);
      return nullptr;
    }
    const std::string alias(parser.current_token().value);
    parser.advance();
    get_module_table().add_module_alias(scope, alias, resolved_path);
  } else if (parser.match(TokenName::AG_LEFT_BRACE)) {
    if (!parser.check(TokenName::AG_RIGHT_BRACE)) {
      while (true) {
        if (!parser.check(TokenName::AG_IDENTIFIER)) {
          print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedIdentifier,
                      parser);
          return nullptr;
        }

        const std::string name(parser.current_token().value);
        parser.advance();
        get_module_table().add_symbol_alias(scope, name, resolved_path + "::" + name);

        if (!parser.match(TokenName::AG_COMMA))
          break;
        if (parser.check(TokenName::AG_RIGHT_BRACE))
          break;
      }
    }

    if (!parser.expect(TokenName::AG_RIGHT_BRACE, error::ExpectedRightBrace))
      return nullptr;
  } else {
    get_module_table().add_module_alias(scope, last_path_segment(resolved_path), resolved_path);
  }

  if (!parser.expect(TokenName::AG_SEMICOLON, error::ExpectedSemicolon))
    return nullptr;

  return import_stmt;
}
} // namespace ag

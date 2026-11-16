#include "Statements/module.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ag {
namespace {
std::string
qualify_module_path(const Parser& parser, std::string_view path) {
  const std::string& scope = parser.current_module_path();
  if (scope.empty() || path == scope)
    return std::string(path);

  if (path.size() > scope.size() &&
      path.compare(0, scope.size(), scope) == 0 &&
      path.compare(scope.size(), 2, "::") == 0)
    return std::string(path);

  return scope + "::" + std::string(path);
}

std::optional<std::filesystem::path>
find_module_file(const Parser& parser, const std::string& relative_path) {
  const std::filesystem::path filename(relative_path);
  const std::filesystem::path current_file(parser.get_filename());

  std::vector<std::filesystem::path> candidates;
  candidates.push_back(current_file.parent_path() / filename);
  for (const auto& include_path : parser.include_paths())
    candidates.push_back(std::filesystem::path(include_path) / filename);
  candidates.emplace_back(filename);

  std::error_code error;
  for (const auto& candidate : candidates) {
    if (std::filesystem::is_regular_file(candidate, error) && !error)
      return candidate;
    error.clear();
  }
  return std::nullopt;
}
} // namespace

ModuleStatement::~ModuleStatement() = default;

AG_scope<Statements>
ModuleStatement::ParseModule(Parser& parser) {
  const Token module_token = parser.current_token();
  parser.advance(); // `mod`
  parser.take_pending_public(); // module visibility is not enforced yet

  auto module_stmt        = AG_cscope<ModuleStatement>();
  module_stmt->m_location = {module_token.line, module_token.column};

  std::vector<std::string> components;
  std::string              name;
  AG_EXPECT_GET_ID_OR_RETURN(
      parser, error::ExpectedModuleName, name, nullptr, TokenName::AG_IDENTIFIER);
  components.push_back(name);

  std::string requested_path = name;
  while (parser.match(TokenName::AG_DOUBLE_COLON)) {
    if (!parser.check(TokenName::AG_IDENTIFIER)) {
      print_error(parser.is_at_end() ? error::UnexpectedEndOfFile : error::ExpectedModuleName,
                  parser);
      return nullptr;
    }
    name = parser.current_token().value;
    parser.advance();
    components.push_back(name);
    requested_path += "::";
    requested_path += name;
  }

  // `mod ag::String;` is a file part of the `ag` namespace: it loads
  // `ag/String.ag`, but declarations inside it are named `ag::Foo`, not
  // `ag::String::Foo`.  Inline `mod ag::String { ... }` keeps the normal
  // nested-module meaning.
  const bool  is_inline_module = parser.check(TokenName::AG_LEFT_BRACE);
  std::string module_file_name;
  if (components.size() > 1 && !is_inline_module) {
    std::string parent_path;
    for (size_t i = 0; i + 1 < components.size(); ++i) {
      if (!parent_path.empty())
        parent_path += "::";
      parent_path += components[i];
    }

    module_stmt->m_path = qualify_module_path(parser, parent_path);

    for (size_t i = 0; i < components.size(); ++i) {
      if (i != 0)
        module_file_name += "/";
      module_file_name += components[i];
    }
    module_file_name += ".ag";

    get_module_table().add_module_part(qualify_module_path(parser, requested_path),
                                       module_stmt->m_path);
  } else {
    if (components.size() > 1)
      module_stmt->m_path = qualify_module_path(parser, requested_path);
    else
      module_stmt->m_path = parser.qualify_name(requested_path);
    module_file_name = name + ".ag";
  }

  get_module_table().declare_module(module_stmt->m_path);

  if (parser.match(TokenName::AG_SEMICOLON)) {
    const auto module_file = find_module_file(parser, module_file_name);
    if (!module_file) {
      print_error("\"Could not open module file\"", parser, module_stmt->m_location);
      return nullptr;
    }

    // The same module file can legitimately be named by several importers.
    // Parse it only once; later `mod` statements become empty.
    std::error_code    canonical_error;
    std::string        module_key = std::filesystem::weakly_canonical(*module_file,
                                                              canonical_error)
                                        .string();
    if (canonical_error)
      module_key = module_file->string();

    if (get_module_table().is_module_file_loaded(module_key))
      return module_stmt;
    get_module_table().mark_module_file_loaded(std::move(module_key));

    std::ifstream file(*module_file, std::ios::binary);
    if (!file.is_open()) {
      print_error("\"Could not open module file\"", parser, module_stmt->m_location);
      return nullptr;
    }

    std::stringstream source;
    source << file.rdbuf();
    module_stmt->m_source = source.str();

    module_stmt->m_lexer = std::make_unique<Lexer>();
    module_stmt->m_lexer->set_source_code(module_stmt->m_source);
    module_stmt->m_lexer->set_filename(module_file->string());
    module_stmt->m_lexer->tokenize();

    parser.parse_module_tokens(*module_stmt->m_lexer,
                               module_stmt->m_path,
                               module_stmt->m_statements);
    return module_stmt;
  }

  AG_EXPECT_OR_RETURN(parser, TokenName::AG_LEFT_BRACE, error::ExpectedLeftBrace, nullptr);

  parser.push_module_path(module_stmt->m_path);

  while (!parser.check(TokenName::AG_RIGHT_BRACE)) {
    if (parser.is_at_end()) {
      parser.pop_module_path();
      print_error(error::UnexpectedEndOfFile, parser);
      return nullptr;
    }

    const size_t position_before_statement = parser.get_current_position();
    auto         statement                 = parser.parse_statement();
    if (!statement) {
      parser.pop_module_path();
      return nullptr;
    }

    module_stmt->m_statements.emplace_back(std::move(statement));

    if (parser.get_current_position() == position_before_statement) {
      parser.pop_module_path();
      print_error(error::UnexpectedToken, parser);
      return nullptr;
    }
  }

  parser.pop_module_path();
  parser.advance(); // `}`
  return module_stmt;
}

bool
ModuleStatement::analyze_semantics(Parser& parser) {
  Lexer* previous_lexer = nullptr;
  if (m_lexer)
    previous_lexer = parser.swap_lexer(m_lexer.get());

  parser.push_module_path(m_path);

  bool valid = true;
  for (const auto& statement : m_statements) {
    if (!statement->analyze_semantics(parser)) {
      valid = false;
      break;
    }
  }

  parser.pop_module_path();
  if (m_lexer)
    parser.swap_lexer(previous_lexer);

  return valid;
}

void
ModuleStatement::generate_ir(LabelCounters& counter,
                             Parser&        parser) {
  Lexer* previous_lexer = nullptr;
  if (m_lexer)
    previous_lexer = parser.swap_lexer(m_lexer.get());

  parser.push_module_path(m_path);
  for (const auto& statement : m_statements)
    statement->generate_ir(counter, parser);
  parser.pop_module_path();

  if (m_lexer)
    parser.swap_lexer(previous_lexer);
}
} // namespace ag

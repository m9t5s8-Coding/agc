#include "Statements/module.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include "macro.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace ag {
ModuleStatement::~ModuleStatement() = default;

AG_scope<Statements>
ModuleStatement::ParseModule(Parser& parser) {
  const Token module_token = parser.current_token();
  parser.advance(); // `mod`
  parser.take_pending_public(); // module visibility is not enforced yet

  auto module_stmt        = AG_cscope<ModuleStatement>();
  module_stmt->m_location = {module_token.line, module_token.column};

  std::string name;
  AG_EXPECT_GET_ID_OR_RETURN(
      parser, error::ExpectedModuleName, name, nullptr, TokenName::AG_IDENTIFIER);

  module_stmt->m_path = parser.qualify_name(name);
  get_module_table().declare_module(module_stmt->m_path);

  if (parser.match(TokenName::AG_SEMICOLON)) {
    const std::filesystem::path current_file(parser.get_filename());
    const std::filesystem::path module_file =
        current_file.parent_path() / (name + ".ag");

    std::ifstream file(module_file, std::ios::binary);
    if (!file.is_open()) {
      print_error("\"Could not open module file\"", parser, module_stmt->m_location);
      return nullptr;
    }

    std::stringstream source;
    source << file.rdbuf();
    module_stmt->m_source = source.str();

    module_stmt->m_lexer = std::make_unique<Lexer>();
    module_stmt->m_lexer->set_source_code(module_stmt->m_source);
    module_stmt->m_lexer->set_filename(module_file.string());
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

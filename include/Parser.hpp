#pragma once
#include "IR/Instructions.hpp"
#include "Lexer.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include "error_msg.hpp"
#include <algorithm>
#include <deque>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace ag {
using TokensVector = std::vector<Token>;
enum class OutputFormat { JSON, TEXT, NONE };
class Parser {
public:
  // Set Lexer so that we can use later
  void
  set_lexer(Lexer& lexer) {
    m_lexer  = &lexer;
    m_tokens = std::move(lexer.get_tokens());

    // EOF is represented by a synthetic token so diagnostics at the end of
    // the file still have a useful line and column.  When the file ends with a
    // newline, point at the end of the last non-empty line instead of a blank
    // line that does not exist in the source.
    m_EOToken.token_name = TokenName::AG_EOF;
    m_EOToken.value      = {};
    m_EOToken.line       = lexer.get_current_line();
    m_EOToken.column     = lexer.get_current_column();
    while (m_EOToken.line > 1 && lexer.get_line_text(m_EOToken.line).empty()) {
      --m_EOToken.line;
      m_EOToken.column = static_cast<uint32_t>(lexer.get_line_text(m_EOToken.line).size());
    }

    if (lexer.has_error()) {
      m_error_message += lexer.get_error_message();
      m_invalid_syntax = true;
    }
  }

  // Get Lexer for using its features
  const Lexer&
  get_lexer() const {
    return *m_lexer;
  }

  // Get the current processing file
  const std::string&
  get_filename() const {
    return m_lexer->get_filename();
  }

  // Get the token that cursor is present
  const Token&
  current_token() const {
    return is_valid() ? m_tokens[m_index] : m_EOToken;
  }

  // Get the token that is next to cursor
  const Token&
  peek_token(size_t offset = 1) const {
    return is_valid(offset) ? m_tokens[m_index + offset] : m_EOToken;
  }

  bool
  peek_token_name(TokenName* token_name) const {
    if (!is_valid())
      return false;
    *token_name = peek_token().token_name;
    return true;
  }

  void
  set_output_file(const std::string& output_file) {
    m_output_file = output_file;
  }

  void
  set_output_format(OutputFormat format) {
    m_output_format = format;
  }

  // Check the current token
  bool
  check(TokenName token_name) const {
    return current_token().token_name == token_name;
  }

  void
  append_error_message(const std::string& message) {
    m_error_message += message;
  }

  const std::string&
  get_error_message() const {
    return m_error_message;
  }

  // Textual IR produced by the IR generation phase.
  const std::string&
  get_ir() const {
    return m_ir_text;
  }

  // Final assembly produced by the code generation phase.
  const std::string&
  get_assembly() const {
    return m_assembly_text;
  }

  // Textual LLVM IR produced by the optional LLVM backend.
  const std::string&
  get_llvm_ir() const {
    return m_llvm_text;
  }

  // LLVM is the executable backend.  Its textual IR is generated on demand
  // for -o and --llvm-ir.
  void
  set_emit_llvm(bool enabled) {
    m_emit_llvm = enabled;
  }

  // Native NASM assembly is kept only for the JSON debug output.  Executable
  // builds no longer run the native backend.
  void
  set_emit_native_assembly(bool enabled) {
    m_emit_native_assembly = enabled;
  }

  // Linear IR instructions after the IR generation phase.  The LLVM backend
  // translates these instructions without needing to walk the AST again.
  const std::vector<AG_scope<Instructions>>&
  get_instructions() const {
    return m_instructions;
  }

  std::string
  to_json() const;

  // Check any one of token and return matched one
  bool
  check_one_of(std::initializer_list<TokenName> tokens,
               TokenName*                       matched = nullptr) const {
    auto c_token_name = current_token().token_name;
    for (auto token : tokens) {
      if (c_token_name == token) {
        if (matched)
          *matched = token;
        return true;
      }
    }
    return false;
  }

  // Advance the cursor, but never move past the synthetic EOF token.
  void
  advance() {
    if (m_index < tokens_size())
      ++m_index;
  }

  // Match the current token advance if matched
  bool
  match(TokenName token_name) {
    if (check(token_name)) {
      advance();
      return true;
    }
    return false;
  }

  // Match the one token adn advance if matched
  bool
  match_one_of(std::initializer_list<TokenName> tokens,
               TokenName*                       matched = nullptr) {
    if (check_one_of(tokens, matched)) {
      advance();
      return true;
    }
    return false;
  }

  // Expect token and if not print error
  bool
  expect(TokenName              token,
         const std::string_view error_message) {
    if (!match(token)) {
      print_error(is_at_end() ? error::UnexpectedEndOfFile : error_message, *this);
      return false;
    }
    return true;
  }

  // Expect any one token and print if not matched
  bool
  expect_one_of(std::initializer_list<TokenName> tokens,
                const std::string_view           error_message,
                TokenName*                       matched = nullptr) {
    if (!match_one_of(tokens, matched)) {
      print_error(is_at_end() ? error::UnexpectedEndOfFile : error_message, *this);
      return false;
    }
    return true;
  }

  // Get current cursor position
  size_t
  get_current_position() const {
    return m_index;
  }

  // Add the statements AST
  bool
  add_statements(AG_scope<Statements> ptr) {
    if (ptr) {
      m_statements.emplace_back(std::move(ptr));
      return true;
    }
    return false;
  }

  // Is the valid cursor
  bool
  is_valid(size_t offset = 0) const {
    return (m_index + offset) < tokens_size();
  }

  // True once all real tokens have been consumed.
  bool
  is_at_end() const {
    return !is_valid() || current_token().token_name == TokenName::AG_EOF;
  }

  // Is the syntax valid
  bool
  is_syntax_valid() const {
    return m_invalid_syntax == false;
  }

  // Return-type context for the function body currently being checked.
  void
  push_function_return_type(const DataType& type) {
    m_function_return_types.push_back(type);
  }

  void
  pop_function_return_type() {
    if (!m_function_return_types.empty())
      m_function_return_types.pop_back();
  }

  const DataType*
  current_function_return_type() const {
    return m_function_return_types.empty() ? nullptr : &m_function_return_types.back();
  }

  // Operator bodies must not resolve their own implicit operator syntax back
  // to themselves.  The active label is used as a recursion guard while the
  // body is semantically checked.
  void
  push_operator_context(const std::string& label) {
    m_active_operators.push_back(label);
  }

  void
  pop_operator_context() {
    if (!m_active_operators.empty())
      m_active_operators.pop_back();
  }

  bool
  is_operator_context_active(std::string_view label) const {
    return std::find(m_active_operators.begin(), m_active_operators.end(), label) !=
           m_active_operators.end();
  }

  // The parser is inside an `impl Type { ... }` block while method signatures
  // are parsed.  This lets `self` get the enclosing struct type without making
  // it a reserved word.
  void
  push_impl_type(std::string name) {
    m_impl_types.push_back(std::move(name));
  }

  void
  pop_impl_type() {
    if (!m_impl_types.empty())
      m_impl_types.pop_back();
  }

  const std::string&
  current_impl_type() const {
    static const std::string empty;
    return m_impl_types.empty() ? empty : m_impl_types.back();
  }

  // The parser is inside a `mod name { ... }` block while its declarations
  // are parsed.  Declarations are stored under a qualified source name so
  // different modules may reuse the same function or type name.
  void
  push_module_path(std::string path) {
    m_module_paths.push_back(std::move(path));
  }

  void
  pop_module_path() {
    if (!m_module_paths.empty())
      m_module_paths.pop_back();
  }

  const std::string&
  current_module_path() const {
    static const std::string empty;
    return m_module_paths.empty() ? empty : m_module_paths.back();
  }

  std::string
  qualify_name(std::string_view name) const {
    if (current_module_path().empty())
      return std::string(name);
    return current_module_path() + "::" + std::string(name);
  }

  // `pub` is parsed by the generic statement dispatcher before the actual
  // declaration parser runs.  The flag is consumed by the declaration that
  // owns the visibility, and cleared afterwards so it cannot leak into a
  // nested declaration.
  void
  set_pending_public(bool value) {
    m_pending_public = value;
  }

  bool
  take_pending_public() {
    const bool value = m_pending_public;
    m_pending_public = false;
    return value;
  }

  // Resolve an unqualified or module-qualified source name through the import
  // bindings of the active module.  Function calls and type references both
  // use this path so `math::sqrt`, `m::sqrt`, and `import math { sqrt }`
  // spellings all resolve to the same declaration.
  std::string
  resolve_symbol_name(std::string_view name) const;

  std::string
  resolve_type_name(std::string_view name) const;

  // Parser-owned storage for synthesized names such as `operator+`.
  std::string_view
  intern_operator_name(const std::string& name) {
    m_operator_names.push_back(name);
    return m_operator_names.back();
  }

  std::string_view
  intern_name(const std::string& name) {
    m_operator_names.push_back(name);
    return m_operator_names.back();
  }

  // Parse the top-level statements of a module file using this parser's
  // tables and diagnostics.  The caller owns the lexer and its source text so
  // token views remain valid after the parser state is restored.
  void
  parse_module_tokens(Lexer&                             lexer,
                      std::string                        module_path,
                      std::vector<AG_scope<Statements>>& statements);

  // Module statements temporarily make their own lexer active while their
  // bodies are analyzed or lowered so diagnostics point at the module source.
  Lexer*
  swap_lexer(Lexer* lexer) {
    Lexer* previous = m_lexer;
    m_lexer         = lexer;
    return previous;
  }

  void
  add_instructios(AG_scope<Instructions> instruction);

  // Parse the Tokens
  void
  parse();

  // Parse the statements
  AG_scope<Statements>
  parse_statement();

private:
  // Get Tokens Size
  size_t
  tokens_size() const {
    return m_tokens.size();
  }

private:
  Lexer*       m_lexer = nullptr;
  Token        m_EOToken;
  TokensVector m_tokens;
  size_t       m_index          = 0;
  bool         m_invalid_syntax = false;
  std::string  m_output_file;
  std::string  m_error_message;
  std::string  m_ir_text;
  std::string  m_assembly_text;
  std::string  m_llvm_text;
  bool         m_emit_llvm = false;
  bool         m_emit_native_assembly = false;
  OutputFormat m_output_format = OutputFormat::NONE;

  std::vector<DataType> m_function_return_types;
  std::vector<std::string> m_active_operators;
  std::vector<std::string> m_impl_types;
  std::vector<std::string> m_module_paths;
  std::deque<std::string>  m_operator_names;
  bool                     m_pending_public = false;

  std::vector<AG_scope<Statements>>   m_statements;
  std::vector<AG_scope<Instructions>> m_instructions;
};

} // namespace ag

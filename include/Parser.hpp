#pragma once
#include "Lexer.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"
#include "Token.hpp"
#include <initializer_list>
#include <vector>

namespace ag {
using TokensVector = std::vector<Token>;
class Parser {
public:
  // Set Lexer so that we can use later
  void
  set_lexer(Lexer& lexer) {
    m_lexer  = &lexer;
    m_tokens = std::move(lexer.get_tokens());
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

  // Check the current token
  bool
  check(TokenName token_name) const {
    return current_token().token_name == token_name;
  }

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

  // Advance the cursor
  void
  advance() {
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
      print_error(error_message, *this);
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
      print_error(error_message, *this);
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

  // Is the syntax valid
  bool
  is_syntax_valid() const {
    return m_invalid_syntax == false;
  }

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

  std::vector<AG_scope<Statements>> m_statements;
};

} // namespace ag

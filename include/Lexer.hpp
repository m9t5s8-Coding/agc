#pragma once

#include "Token.hpp"

#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace ag {
using TokensVector = std::vector<Token>;
class Lexer {
public:
  Lexer();
  void
  set_source_code(const std::string& data) {
    m_source_code = data;
  }

  void
  set_filename(const std::string& filename) {
    m_filename = filename;
  }

  const std::string&
  get_filename() const {
    return m_filename;
  }

  TokensVector&
  get_tokens() {
    return m_tokens;
  }

  void
  tokenize();
  void
  print_tokens() const;

  std::string_view
  get_line_text(size_t line_number) const {
    if (m_line_starts.empty() || line_number == 0)
      return {};

    // Clamp to the last known line so diagnostics at EOF can never index
    // outside the line table.
    size_t line_index = line_number - 1;
    if (line_index >= m_line_starts.size())
      line_index = m_line_starts.size() - 1;

    const size_t start = m_line_starts[line_index];
    size_t       end   = line_index + 1 < m_line_starts.size() ? m_line_starts[line_index + 1] : m_source_code.size();

    if (end > start && end <= m_source_code.size() && m_source_code[end - 1] == '\n')
      --end;

    if (start > end || start > m_source_code.size())
      return {};

    return m_source_code.substr(start, end - start);
  }

  uint32_t
  get_current_line() const {
    return m_current_line;
  }

  uint32_t
  get_current_column() const {
    return m_current_column;
  }

  bool
  has_error() const {
    return !m_error_message.empty();
  }

  const std::string&
  get_error_message() const {
    return m_error_message;
  }

  friend std::ostream&
  operator<<(std::ostream& ss,
             const Lexer&  tokenizer);

private:
  void
  initialize();
  bool
  scan_char(const char c);
  void
  create_string_literal();
  void
  create_char_literal();

  void
  create_keyword(const char c);
  void
  process_word_buffer();
  void
  process_number_buffer();

  void
  add_token(Token&& token) {
    token.line   = m_current_line;
    token.column = m_current_column;
    m_tokens.push_back(std::move(token));
  }
  void
  add_token(Token& token) {
    token.line   = m_current_line;
    token.column = m_current_column;
    m_tokens.push_back(token);
  }
  char
  peek() const {
    return is_valid() ? m_source_code[m_index + 1] : '\0';
  }

  void
  consume() {
    m_buffer_size++;
    m_index++;
    m_current_column++;
  }

  void
  increase() {
    m_current_column++;
    m_index++;
  }

  const char*
  get() const {
    return m_source_code.data() + m_index;
  }
  size_t
  code_size() const {
    return m_source_code.size();
  }
  bool
  is_valid() const {
    return m_index + 1 < code_size();
  }
  void
  clear_buffer() {
    m_buffer_size = 1;
  }
  size_t
  buffer_size() const {
    return m_buffer_size;
  }
  bool
  contains(const std::string_view str_view,
           Token&                 out_token);

  void
  process_new_line() {
    m_current_line++;
    m_current_column = 0;
    m_line_starts.push_back(m_index + 1);
  }

private:
  std::string_view    m_source_code;
  mutable const char* m_buffer;

  // String literals with escape sequences are decoded into owned storage.
  // Token values point here, so this storage must outlive the token list.
  std::deque<std::string> m_decoded_strings;

  std::vector<size_t> m_line_starts;

  size_t       m_buffer_size = 1;
  TokensVector m_tokens;

  uint32_t m_current_line   = 1;
  uint32_t m_current_column = 0;

  size_t m_index = 0;

  using TokenMap = std::unordered_map<std::string, Token>;
  TokenMap m_keyword_map;

  std::string m_filename;
  std::string m_error_message;
};

} // namespace ag

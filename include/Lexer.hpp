#pragma once

#include "Token.hpp"

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
    size_t start = m_line_starts[line_number - 1];
    size_t end   = line_number < m_line_starts.size()
                       ? m_line_starts[line_number] - 1
                       : m_source_code.size();
    if (end > start && m_source_code[end] == '\n')
      --end;

    return m_source_code.substr(start, end - start + 1);
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
  create_stringn_literal();
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

  std::vector<size_t> m_line_starts;

  size_t       m_buffer_size = 1;
  TokensVector m_tokens;

  uint32_t m_current_line   = 1;
  uint32_t m_current_column = 0;

  size_t m_index = 0;

  using TokenMap = std::unordered_map<std::string, Token>;
  TokenMap m_keyword_map;

  std::string m_filename;
};

} // namespace ag

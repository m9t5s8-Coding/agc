#pragma once

#include "Token.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace ag {
using TokensVector = std::vector<Token>;
class Lexer {
public:
  Lexer();
  void set_source_code(std::string& data) { m_source_code = std::move(data); }

  TokensVector& get_tokens() { return m_tokens; }

  void tokenize();
  void print_tokens() const;

  friend std::ostream& operator<<(std::ostream& ss, const Lexer& tokenizer);

private:
  void initialize();
  bool scan_char(const char c);
  void create_stringn_literal();
  void create_char_literal();

  void create_keyword(const char c);
  void process_word_buffer();
  void process_number_buffer();

  void add_token(Token&& token) { m_tokens.push_back(std::move(token)); }
  void add_token(Token& token) { m_tokens.push_back(token); }
  char peek() const { return is_valid() ? m_source_code[m_index + 1] : '\0'; }

  void consume() {
    m_buffer_size++;
    m_index++;
  }

  void increase() { m_index++; }

  const char* get() const { return m_source_code.data() + m_index; }
  size_t      code_size() const { return m_source_code.size(); }
  bool        is_valid() const { return m_index + 1 < code_size(); }
  void        clear_buffer() { m_buffer_size = 1; }
  size_t      buffer_size() const { return m_buffer_size; }
  bool        contains(const std::string_view str_view, Token& out_token);

private:
  std::string         m_source_code;
  mutable const char* m_buffer;

  size_t       m_buffer_size = 1;
  TokensVector m_tokens;

  size_t                                 m_index = 0;
  std::unordered_map<std::string, Token> m_keyword_map;
};

}  // namespace ag

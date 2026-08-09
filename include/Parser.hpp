#pragma once
#include "Lexer.hpp"
#include "Statements/Statements.hpp"

#include <vector>

namespace ag {
using TokensVector = std::vector<Token>;
class Parser {
public:
  void         set_tokens(TokensVector& tokens) { m_tokens = std::move(tokens); }
  const Token& peek_token() const { return is_valid() ? m_tokens[m_index + 1] : m_EOToken; }
  TokenName    peek_token_name() const { return peek_token().token_name; }
  const Token& get_current_token() const { return is_valid() ? m_tokens[m_index] : m_EOToken; }

  void add_statements(AG_scope<Statements>& ptr) {
    if (ptr) {
      m_statemets.emplace_back(std::move(ptr));
    }
  }
  bool expect_token(TokenName token_name) const {
    return is_valid() ? m_tokens[m_index + 1].token_name == token_name : false;
  }
  bool consume_token() {
    m_index++;
    return is_valid();
  }
  void increase_token() { m_index++; }
  bool is_valid() const { return (m_index + 1) < tokens_size(); }

  void parse();

private:
  bool scan_token(const Token& token);

  size_t tokens_size() const { return m_tokens.size(); }

private:
  Token                             m_EOToken;
  TokensVector                      m_tokens;
  size_t                            m_index = 0;
  std::vector<AG_scope<Statements>> m_statemets;
};

}  // namespace ag

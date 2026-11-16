#pragma once
#include "Expressions/Expression.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class Parser;
class Expression;
class WhileStatement : public Statements {
public:
  WhileStatement() {}
  virtual ~WhileStatement() {}

  static AG_scope<Statements>
  ParseWhile(Parser& parser);

  static AG_scope<Statements>
  ParseUntil(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

  virtual void
  set_loop_label(std::string label) override {
    m_label = std::move(label);
  }

private:
  static AG_scope<Statements>
  parse_loop(Parser& parser,
             bool    until);

  AG_scope<Expression> m_condition;
  AG_scope<Statements> m_while_body;
  bool                 m_until = false;
  std::string          m_label;

  // `while let pattern = expression { ... }`
  bool         m_is_let = false;
  MatchPattern m_pattern;
};
} // namespace ag

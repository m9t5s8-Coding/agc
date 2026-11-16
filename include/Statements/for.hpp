#pragma once

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

#include <string>
#include <vector>

namespace ag {
class Parser;
class Expression;

// `for name in start..end { ... }`, `for name in start..=end by step { ... }`,
// and iteration over a pointer whose declared array length is known.
// tuple/destructuring and general iterator protocols are not implemented yet.
class ForStatement : public Statements {
public:
  ForStatement() {}
  virtual ~ForStatement() {}

  static AG_scope<Statements>
  ParseFor(Parser& parser);

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
  std::vector<std::string> m_names;
  AG_scope<Expression>     m_iterable;
  AG_scope<Expression>     m_step;
  AG_scope<Statements>     m_body;
  std::string              m_label;

  bool     m_is_range     = false;
  bool     m_is_variadic  = false;
  bool     m_is_map       = false;
  DataType m_item_type;
  DataType m_key_type;
  size_t   m_iteration_count = 0;
};
} // namespace ag

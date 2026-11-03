#pragma once

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

#include <string>
#include <vector>

namespace ag {
class Parser;

// impl Point {
//   func dist(self) -> int { ... }
// }
class ImplStatement : public Statements {
public:
  virtual ~ImplStatement() {}

  static AG_scope<Statements>
  ParseImpl(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  std::string                    m_type_name;
  std::vector<AG_scope<Statements>> m_methods;
};
} // namespace ag

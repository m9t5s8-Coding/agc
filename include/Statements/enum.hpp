#pragma once

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Parser;

// enum Status {
//   Ok,
//   Error(string),
//   Moved { x: int, y: int },
// }
class EnumStatement : public Statements {
public:
  virtual ~EnumStatement() {}

  static AG_scope<Statements>
  ParseEnum(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  EnumDefinition m_definition;
};
} // namespace ag

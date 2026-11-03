#pragma once

#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Parser;

// struct Point {
//   x: int,
//   y: int,
// }
//
// Fields are laid out as pointer-sized slots.  A struct value is represented
// by the address of its storage, matching the pointer-sized values used by the
// rest of the language.
class StructStatement : public Statements {
public:
  virtual ~StructStatement() {}

  static AG_scope<Statements>
  ParseStruct(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  StructDefinition m_definition;
};
} // namespace ag

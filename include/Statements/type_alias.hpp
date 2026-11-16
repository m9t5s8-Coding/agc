#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Parser;

// `type Meters = float;`
//
// Aliases are resolved while parsing, so declarations that appear later in the
// file can use the alias in a type position.  Function-type aliases such as
// `type Callback = func(int) -> void;` are accepted as opaque named types for
// now; calling through them is not implemented.
class TypeAliasStatement : public Statements {
public:
  virtual ~TypeAliasStatement() {}

  static AG_scope<Statements>
  ParseType(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override {}

  virtual bool
  analyze_semantics(Parser&) override {
    return true;
  }
};
} // namespace ag

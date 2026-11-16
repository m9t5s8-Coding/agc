#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;

// import ag::io { println, readln };
// import ag::math as m;
// import ag::fs;
// import ag::String { String, string_new };  // file part of module `ag`
class ImportStatement : public Statements {
public:
  virtual ~ImportStatement() {}

  static AG_scope<Statements>
  ParseImport(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override {}

  virtual bool
  analyze_semantics(Parser&) override {
    return true;
  }
};
} // namespace ag

#pragma once

#include "Statements/Statements.hpp"

namespace ag {
class Parser;

// `test "name" { ... }` declares a test case.  The body is parsed so syntax
// errors are still reported, but tests are not analyzed or executed in a
// normal build; there is no test harness yet.
class TestStatement : public Statements {
public:
  TestStatement() {}
  virtual ~TestStatement() {}

  static AG_scope<Statements>
  ParseTest(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  AG_scope<Statements> m_body;
};
} // namespace ag

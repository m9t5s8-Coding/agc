
#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Expression;
class Parser;
class VarStatement : public Statements {
public:
  virtual ~VarStatement() {}

  static AG_scope<Statements>
  ParseVar(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  DataType             m_datatype;
  std::string_view     m_name;
  AG_scope<Expression> m_expr;
  bool                 m_is_static = false;
  // `var name = value;` infers the type from the initializer.
  bool                 m_type_inferred = false;

  // Set during semantic analysis for module-level declarations.  Globals are
  // stored under this qualified name so two modules may each declare `value`.
  std::string m_qualified_name;
};
} // namespace ag

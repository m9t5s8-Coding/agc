#pragma once
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

namespace ag {
class Expression;
class Parser;
class LetStatement : public Statements {
public:
  virtual ~LetStatement() {}

  static AG_scope<Statements>
  ParseLet(Parser& parser);

  // `const name: Type = value;` is the read-only counterpart of `let`.  It
  // shares the same storage and initializer machinery, but requires an
  // initializer because a constant without a value is meaningless.
  static AG_scope<Statements>
  ParseConst(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

private:
  DataType             m_datatype;
  std::string_view     m_name;
  // `let (a, b) = tuple;` binds one name per tuple slot.
  std::vector<std::string_view> m_pattern_names;
  std::vector<DataType>         m_pattern_types;
  bool                          m_is_destructuring = false;

  // `let Point { x, y } = point;` loads each named field from the struct.
  bool                     m_is_struct_destructuring = false;
  std::string              m_destructure_type;
  std::vector<std::string> m_destructure_fields;
  std::vector<std::string> m_destructure_bindings;
  AG_scope<Expression> m_expr;
  bool                 m_is_static = false;
  // `let name = value;` infers the type from the initializer.
  bool                 m_type_inferred = false;

  // Set during semantic analysis for module-level declarations.  Globals are
  // stored under this qualified name so two modules may each declare `value`.
  std::string m_qualified_name;
};
} // namespace ag

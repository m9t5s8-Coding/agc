#pragma once
#include "Parser.hpp"
#include "Statements/Statements.hpp"
#include "Statements/Utils.hpp"

// func main(num1: int, num2: int) -> int;
// func main(int, int) -> int;
// func main(num1 : int,
//      num2 : int) -> int {
//   return num1 + num2;
// }
// rect1 : Rectangle
// user_defined
// Rectangle
// int
// int

namespace ag {
class FunctionStatement : public Statements {
public:
  virtual ~FunctionStatement() {}

  static AG_scope<Statements>
  ParseFunction(Parser& parser);

  static AG_scope<Statements>
  ParseOperator(Parser& parser);

  virtual void
  generate_ir(LabelCounters&,
              Parser&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

  bool
  register_function(Parser& parser);

  void
  add_parameter(FunctionParameter& param) {
    m_parameters.emplace_back(std::move(param));
  }

  void
  set_function_name(std::string function_name) {
    m_function_name = std::move(function_name);
  }

private:
  using FunctionParameterVec = std::vector<FunctionParameter>;

  std::string
  function_label() const;

  std::string
  source_name() const;

  // Member functions without a `self` parameter are associated functions and
  // can be called as `Type::name(...)`.  `static func` makes this explicit.
  bool
  is_associated() const;

  std::string          m_function_name;
  std::string          m_owner_type;
  std::string          m_module_path;
  FunctionParameterVec m_parameters;
  DataType             m_return_type;
  AG_scope<Statements> m_function_body;
  bool                 is_declaration = false;
  bool                 m_is_operator  = false;
  bool                 m_is_public    = false;
  bool                 m_is_static    = false;
  // Set while parsing an `extern "c" { ... }` block.  External declarations
  // keep their raw C symbol name and are resolved by the system linker.
  bool                 m_is_extern    = false;
};
} // namespace ag

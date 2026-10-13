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

  virtual void
  generate(CodeGenContext&) override;

  virtual bool
  analyze_semantics(Parser& parser) override;

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

  std::string          m_function_name;
  FunctionParameterVec m_parameters;
  DataType             m_return_type;
  AG_scope<Statements> m_function_body;
  bool                 is_declaration = false;
};
} // namespace ag

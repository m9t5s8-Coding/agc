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

struct DataType_Data {
  DATA_TYPES       datatype_type = DATA_TYPES::VOID;
  std::string_view datatype_name;
};

struct FunctionParam {
  std::string_view variable_name;
  DataType_Data    data_type;
};

class FunctionStatement : public Statements {
public:
  virtual ~FunctionStatement() {}

  static AG_scope<Statements>
  ParseFunction(Parser& parser);

  virtual void
  generate(CodeGenContext&) override;

  void
  add_parameter(FunctionParam& param) {
    m_parameters.emplace_back(std::move(param));
  }

  void
  set_function_name(std::string function_name) {
    m_function_name = std::move(function_name);
  }

private:
  using FunctionParameter = std::vector<FunctionParam>;

  std::string          m_function_name;
  FunctionParameter    m_parameters;
  DataType_Data        m_return_type;
  AG_scope<Statements> m_function_body;
  bool                 is_declaration = false;
};
} // namespace ag

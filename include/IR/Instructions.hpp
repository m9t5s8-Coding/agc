#pragma once
#include "Statements/Statements.hpp"
#include <string>

namespace ag {
class Instructions {
public:
  explicit Instructions(int index) : m_function_index(index) {}
  virtual ~Instructions() = default;

  virtual std::string
  to_string() = 0;

  virtual void
  generate_code(CodeGenContext&) = 0;

protected:
  int m_function_index{-1};
};
} // namespace ag

#pragma once
#include "Statements/Statements.hpp"
#include <string>

namespace ag {
class Instructions {
public:
  virtual ~Instructions() = default;

  virtual std::string
  to_string() = 0;

  virtual void
  generate_code(CodeGenContext&) = 0;
};
} // namespace ag

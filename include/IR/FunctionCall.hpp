#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class FunctionCall : public Instructions {
public:
  FunctionCall(const std::string& name) : m_name(name) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_name;
};
} // namespace ag

#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class FunctionCall : public Instructions {
public:
  FunctionCall(const std::string& name,
               int                index  = -1,
               const std::string& result = {})
      : Instructions(index),
        m_name(name),
        m_result(result) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_name;
  std::string m_result;
};
} // namespace ag

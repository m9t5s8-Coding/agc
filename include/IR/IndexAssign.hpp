#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class IndexAssign : public Instructions {
public:
  IndexAssign(const std::string& object,
              const std::string& index,
              const std::string& value,
              int                function_index)
      : Instructions(function_index),
        m_object(object),
        m_index(index),
        m_value(value) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_object, m_index, m_value;
};
} // namespace ag

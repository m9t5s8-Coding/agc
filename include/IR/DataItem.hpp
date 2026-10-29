#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class DataItem : public Instructions {
public:
  DataItem(const std::string& name,
           const std::string& value,
           int                index = -1)
      : Instructions(index),
        m_name(name),
        m_value(value) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_name, m_value;
};
} // namespace ag

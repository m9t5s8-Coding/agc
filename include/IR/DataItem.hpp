#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class DataItem : public Instructions {
public:
  DataItem(const std::string& name,
           const std::string& value)
      : m_name(name),
        m_value(value) {}

  virtual std::string
  to_string() override;

  std::string m_name, m_value;
};
} // namespace ag

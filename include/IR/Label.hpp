#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class Label : public Instructions {
public:
  Label(const std::string& name) : m_name(name) {}

  virtual std::string
  to_string() override;

  std::string m_name;
};
} // namespace ag

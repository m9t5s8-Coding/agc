#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class Jump : public Instructions {
public:
  Jump(const std::string& name) : m_name(name) {}

  virtual std::string
  to_string() override;

  std::string m_name;
};
} // namespace ag

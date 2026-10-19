#pragma once
#include "IR/Instructions.hpp"

namespace ag {
class Assign : public Instructions {
public:
  Assign(const std::string& source,
         const std::string& destination)
      : m_source(source),
        m_destination(destination) {}

  virtual std::string
  to_string() override;

  std::string m_source, m_destination;
};
} // namespace ag

#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
class Assign : public Instructions {
public:
  Assign(const std::string& source,
         const std::string& destination,
         int                index)
      : Instructions(index),
        m_source(source),
        m_destination(destination) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_source, m_destination;
};
} // namespace ag

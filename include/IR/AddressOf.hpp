#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <string>

namespace ag {
// Store the address of a variable or global slot into a temporary.
//
// Member and index expressions are lowered by the AST into ordinary pointer
// arithmetic, so this instruction only needs to know how to take the address
// of named storage.
class AddressOf : public Instructions {
public:
  AddressOf(const std::string& store_to,
            const std::string& source,
            int                index)
      : Instructions(index),
        m_store_to(store_to),
        m_source(source) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_store_to;
  std::string m_source;
};
} // namespace ag

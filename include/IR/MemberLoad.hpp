#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstdint>
#include <string>

namespace ag {
// Load one pointer-sized field from a struct value.
class MemberLoad : public Instructions {
public:
  MemberLoad(const std::string& store_to,
             const std::string& object,
             uint32_t           byte_offset,
             int                index)
      : Instructions(index),
        m_store_to(store_to),
        m_object(object),
        m_byte_offset(byte_offset) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_store_to;
  std::string m_object;
  uint32_t    m_byte_offset = 0;
};
} // namespace ag

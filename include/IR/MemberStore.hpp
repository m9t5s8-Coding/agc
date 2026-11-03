#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstdint>
#include <string>

namespace ag {
// Store one pointer-sized field into a struct value.
class MemberStore : public Instructions {
public:
  MemberStore(const std::string& object,
              uint32_t           byte_offset,
              const std::string& value,
              int                index)
      : Instructions(index),
        m_object(object),
        m_byte_offset(byte_offset),
        m_value(value) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_object;
  uint32_t    m_byte_offset = 0;
  std::string m_value;
};
} // namespace ag

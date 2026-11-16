#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ag {
// Store one field into a struct value using the field's storage width.
class MemberStore : public Instructions {
public:
  MemberStore(const std::string& object,
              uint32_t           byte_offset,
              const std::string& value,
              int                index,
              size_t             width     = 8,
              bool               is_signed = true)
      : Instructions(index),
        m_object(object),
        m_byte_offset(byte_offset),
        m_value(value),
        m_width(width == 0 ? 8 : width),
        m_is_signed(is_signed) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_object;
  uint32_t    m_byte_offset = 0;
  std::string m_value;
  size_t      m_width       = 8;
  bool        m_is_signed   = true;
};
} // namespace ag

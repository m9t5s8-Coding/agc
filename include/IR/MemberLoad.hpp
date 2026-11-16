#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace ag {
// Load one field from a struct value.  `m_width` is the field's storage width;
// it defaults to the historical pointer-sized slot.
class MemberLoad : public Instructions {
public:
  MemberLoad(const std::string& store_to,
             const std::string& object,
             uint32_t           byte_offset,
             int                index,
             size_t             width     = 8,
             bool               is_signed = true)
      : Instructions(index),
        m_store_to(store_to),
        m_object(object),
        m_byte_offset(byte_offset),
        m_width(width == 0 ? 8 : width),
        m_is_signed(is_signed) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_store_to;
  std::string m_object;
  uint32_t    m_byte_offset = 0;
  size_t      m_width       = 8;
  bool        m_is_signed   = true;
};
} // namespace ag

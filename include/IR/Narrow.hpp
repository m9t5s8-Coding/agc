#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstddef>
#include <string>

namespace ag {
// Narrow a value to a fixed-width storage size and return it in an i64 slot.
//
// AGC keeps every local in an eight-byte slot.  For the sized integer spellings
// (`u8`, `i8`, `u16`, `i16`, `u32`, `i32`, and `bool`) an assignment must still
// wrap/truncate to the declared width so a variable behaves like its C
// counterpart.  The value is truncated to `m_width` bytes and then sign- or
// zero-extended back into the slot.
class Narrow : public Instructions {
public:
  Narrow(const std::string& destination,
         const std::string& value,
         size_t             width,
         bool               is_signed,
         int                index)
      : Instructions(index),
        m_destination(destination),
        m_value(value),
        m_width(width == 0 ? 8 : width),
        m_is_signed(is_signed) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_destination;
  std::string m_value;
  size_t      m_width     = 8;
  bool        m_is_signed = true;
};
} // namespace ag

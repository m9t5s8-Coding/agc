#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstddef>
#include <string>

namespace ag {
// Store a value through an address using the pointee's storage width.
class DerefStore : public Instructions {
public:
  DerefStore(const std::string& address,
             const std::string& value,
             bool               is_byte,
             int                index)
      : Instructions(index),
        m_address(address),
        m_value(value),
        m_is_byte(is_byte),
        m_width(is_byte ? 1 : 8) {}

  DerefStore(const std::string& address,
             const std::string& value,
             int                index,
             size_t             width,
             bool               is_signed)
      : Instructions(index),
        m_address(address),
        m_value(value),
        m_is_byte(width == 1),
        m_width(width == 0 ? 8 : width),
        m_is_signed(is_signed) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_address;
  std::string m_value;
  bool        m_is_byte = false;
  size_t      m_width   = 8;
  bool        m_is_signed = true;
};
} // namespace ag

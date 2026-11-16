#pragma once

#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

#include <cstddef>
#include <string>

namespace ag {
// Load the value pointed to by an address.  `m_width` records the storage
// width of the pointee so `bool`/`u8`/`i8` read one byte, `i16`/`u16` two,
// and the ordinary pointer-sized types keep the historical eight-byte load.
class DerefLoad : public Instructions {
public:
  DerefLoad(const std::string& store_to,
            const std::string& address,
            bool               is_byte,
            int                index)
      : Instructions(index),
        m_store_to(store_to),
        m_address(address),
        m_is_byte(is_byte),
        m_width(is_byte ? 1 : 8) {}

  DerefLoad(const std::string& store_to,
            const std::string& address,
            int                index,
            size_t             width,
            bool               is_signed)
      : Instructions(index),
        m_store_to(store_to),
        m_address(address),
        m_is_byte(width == 1),
        m_width(width == 0 ? 8 : width),
        m_is_signed(is_signed) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_store_to;
  std::string m_address;
  bool        m_is_byte = false;
  size_t      m_width   = 8;
  bool        m_is_signed = true;
};
} // namespace ag

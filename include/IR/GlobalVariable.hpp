#pragma once
#include "IR/Instructions.hpp"
#include "Statements/Statements.hpp"

namespace ag {
// Module-level storage emitted in the native `.data` section and as an LLVM
// global.  Globals are always 8-byte slots so the same load/store lowering can
// be used for integers, pointers, and characters.
class GlobalVariable : public Instructions {
public:
  GlobalVariable(const std::string& name,
                 const std::string& initializer,
                 int                index = -1)
      : Instructions(index),
        m_name(name),
        m_initializer(initializer) {}

  virtual std::string
  to_string() override;

  virtual void
  generate_code(CodeGenContext&) override;

  std::string m_name;
  std::string m_initializer;
};
} // namespace ag

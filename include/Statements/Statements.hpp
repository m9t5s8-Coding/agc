#pragma once

#include "Statements/Utils.hpp"
#include <cstdint>
#include <memory>
#include <sstream>

namespace ag {
template <typename T>
using AG_ref = std::shared_ptr<T>;

template <typename T>
using AG_scope = std::unique_ptr<T>;

template <typename T,
          typename... Args>
constexpr AG_ref<T>
AG_cref(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T,
          typename... Args>
constexpr AG_scope<T>
AG_cscope(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

struct CodeGenContext {
  std::stringstream text;
  std::stringstream data;
  std::stringstream bss;
  std::stringstream rodata;
  std::stringstream syscall;
};

struct LabelCounters {
  uint32_t label        = 0;
  uint32_t string_label = 0;
  uint32_t temp_label   = 0;
  uint32_t block_label  = 0;
};

enum class EXPR_TYPE { LITERAL, IDENTIFIER, COMPLEX_RUNTIME };
class Parser;
class Statements {
public:
  Statements() {}
  virtual ~Statements() {}

  virtual void
  generate_ir(LabelCounters&,
              Parser&) = 0;

  virtual bool
  analyze_semantics(Parser&) = 0;

protected:
  SourceLocation m_location;
};

} // namespace ag

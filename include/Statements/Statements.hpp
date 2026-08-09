#pragma once
#include <memory>
#include <sstream>

namespace ag {
template <typename T>
using AG_ref = std::shared_ptr<T>;

template <typename T>
using AG_scope = std::unique_ptr<T>;

template <typename T, typename... Args>
constexpr AG_ref<T> AG_cref(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
constexpr AG_scope<T> AG_cscope(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

class Statements {
public:
  Statements() {}
  virtual ~Statements() {}

  virtual void generate(std::stringstream& ss) = 0;
};
}  // namespace ag

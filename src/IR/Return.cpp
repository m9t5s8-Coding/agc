#include "IR/Return.hpp"
#include <format>

namespace ag {
std::string
Return::to_string() {
  return std::format("return\n");
}
} // namespace ag

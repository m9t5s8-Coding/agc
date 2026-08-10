#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace ag {
inline int stoi(const std::string& str) {
  int exit_value = -1;
  {
    try {
      exit_value = std::stoi(str);
    } catch (...) {
      exit_value = -1;
    }
  }
  return exit_value;
}

inline std::string string_view_2_string(const std::string_view& str_view) {
  return std::string(str_view);
}

inline void print_error(const char* error) { std::cerr << error << std::endl; }

}  // namespace ag

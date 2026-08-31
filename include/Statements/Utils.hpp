#pragma once

#include <string>
#include <string_view>

namespace ag {

enum class DATA_TYPES {
  VOID,
  INT,
  FLOAT,
  CHAR,
  BOOL,
  USER_DEFINED,
};

inline DATA_TYPES
get_datatypes(const std::string_view keyword) {
  constexpr static std::pair<std::string, DATA_TYPES> m_datatype_map[] = {
      {"int", DATA_TYPES::INT},
      {"void", DATA_TYPES::VOID},
      {"char", DATA_TYPES::CHAR},
      {"float", DATA_TYPES::FLOAT},
      {"bool", DATA_TYPES::BOOL},
  };
  for (const auto& [view, data_type] : m_datatype_map) {
    if (view == keyword) {
      return data_type;
    }
  }
  return DATA_TYPES::USER_DEFINED;
}
//
// using DC_VALUE = uint16_t;
// enum class DATATYPE_COMPATIBILITY : DC_VALUE {
//   NONE = 0,
//   INT_LITERAL = 1 << 0,
//   FLOAT_LITERAL = 1 << 1,
//   HEX_LITERAL = 1 << 2,
//   BINARY_LITERAL = 1 << 3,
//   STRING_LITERAL = 1 << 4,
//   CHAR_LITERAL = 1 << 5,
//   BOOL_LITERAL = 1 << 6,
// };
//
// inline DATATYPE_COMPATIBILITY
// operator|(DATATYPE_COMPATIBILITY lhs,
//           DATATYPE_COMPATIBILITY rhs) {
//   return DATATYPE_COMPATIBILITY((DC_VALUE)lhs | (DC_VALUE)rhs);
// }
//
// (DATA_TYPES::INT, AG::INT_LITERAL) let x : int = 10;
// let str : string = "Hello World!";
// string -> AG_KEYWORD(string) DATA_TYPES::STRING
// TokenName::AG_STRING_LITERAL -> DATATYPE_COMPATIBILITY::STRING_LITERAL
// DATA_TYPES::STRING -> STRING_LITERAL
// AG_KEYWORD("string"), AG_STRING_LIRERAL
// inline bool is_compatible()
//
// inline DATATYPE_COMPATIBILITY
// get_datatype_compatability(DATA_TYPES data_types) {
//   using DC = DATATYPE_COMPATIBILITY;
//   switch (data_types) {
//   case ag::DATA_TYPES::INT: {
//     return DC::INT_LITERAL | DC::BINARY_LITERAL | DC::HEX_LITERAL;
//   }
//
//   case ag::DATA_TYPES::FLOAT: {
//     return DC::INT_LITERAL | DC::BINARY_LITERAL | DC::HEX_LITERAL |
//            DC::FLOAT_LITERAL;
//   }
//
//   case DATA_TYPES::STRING: {
//     return DC::STRING_LITERAL | DC::CHAR_LITERAL;
//   }
//
//   case DATA_TYPES::CHAR: {
//     return DC::CHAR_LITERAL;
//   }
//
//   case DATA_TYPES::BOOL: {
//     return DC::BOOL_LITERAL | DC::INT_LITERAL | DC::BINARY_LITERAL |
//            DC::HEX_LITERAL;
//   }
//
//   default:
//     return DATATYPE_COMPATIBILITY::NONE;
//   }
// }
//
// // bool
// // is_compatible()

inline int
stoi(const std::string& str) {
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

inline std::string
string_view_2_string(const std::string_view& str_view) {
  return std::string(str_view);
}

// example.aero:12:11: error: variable 'x' is not declared
//         write(x, len);
//               ^

class Parser;
void
print_error(const std::string_view error,
            const Parser&          parser);
} // namespace ag

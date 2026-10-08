#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ag {

enum class PrimitiveType { VOID, INT, FLOAT, DOUBLE, CHAR, STRING, BOOL, USER_DEFINED, IDENTIFIER, UNEXPECTED };

enum class TypeKind {
  PRIMITIVE,
  POINTER,
  REFERENCE,
  ARRAY,
  FUNCTION,
  STRUCT,
  ENUM,
};

enum class ValueTiming : uint8_t { COMPILE_TIME, RUNTIME };

enum class StorageClass : uint8_t {
  AUTOMATIC, // default local variable
  STATIC,    // static storage duration, internal linkage (per TU)
  EXTERN,    // external linkage, defined elsewhere
  REGISTER,  // hint only, rarely respected by modern backends
  THREAD_LOCAL,
};

enum class SymbolKind {
  VARIABLE,
  FUNCTION_PARAMETER,
  FUNCTION,
  STRUCT_TYPE,
  ENUM_TYPE,
  ENUM_CONSTANT,
  TYPE_ALIAS,
  NAMESPACE,
};

struct SourceLocation {
  uint32_t line   = 0;
  uint32_t column = 0;
};

struct DataType {
  TypeKind      kind           = TypeKind::PRIMITIVE;
  PrimitiveType primitive_type = PrimitiveType::VOID; // valid iff kind == PRIMITIVE

  std::string_view type_name;

  bool is_const    = false;
  bool is_volatile = false;
  bool is_signed   = true; // relevant for integer PrimitiveType

  std::shared_ptr<DataType> pointee;

  std::optional<size_t> array_length; // nullopt = unsized, e.g. int[]

  std::shared_ptr<DataType>              return_type;
  std::vector<std::shared_ptr<DataType>> parameter_types;
  bool                                   is_variadic = false;

  std::optional<uint32_t> definition_symbol_id;

  size_t size_in_bytes = 0;
  size_t alignment     = 0;

  bool
  operator==(const DataType& other) const;
  bool
  operator!=(const DataType& other) const {
    return !(*this == other);
  }
};

using ConstantValue = std::variant<std::monostate, // no value / not a constant
                                   int64_t,
                                   double,
                                   bool,
                                   char,
                                   std::string>;

struct StructMember {
  std::string    name;
  DataType       type;
  size_t         byte_offset = 0;
  SourceLocation location;
};

struct StructDefinition {
  std::string               name;
  std::vector<StructMember> members;
  size_t                    size_in_bytes = 0;
  size_t                    alignment     = 0;
  SourceLocation            location;
};

struct EnumConstant {
  std::string    name;
  int64_t        value = 0;
  SourceLocation location;
};

struct EnumDefinition {
  std::string               name;
  PrimitiveType             underlying_type = PrimitiveType::INT;
  std::vector<EnumConstant> constants;
  SourceLocation            location;
};

struct Declaration {
  DataType     type;
  ValueTiming  timing  = ValueTiming::RUNTIME;
  StorageClass storage = StorageClass::AUTOMATIC;

  ConstantValue initial_value; // monostate if uninitialized / not constant
  bool          is_initialized = false;

  SourceLocation location;
};

struct SymbolEntry {
  std::string name; // symbol table maps are often keyed by name too,
                    // but keep it here for standalone use (diagnostics,
                    // iteration, "previous declaration was here" etc.)
  SymbolKind kind = SymbolKind::VARIABLE;

  Declaration declaration;

  // --- Scope / codegen bookkeeping ---
  uint32_t scope_id     = 0; // which lexical scope this belongs to
  int64_t  stack_offset = 0; // valid for RUNTIME locals

  // --- Diagnostics / lints ---
  bool is_used = false;

  SourceLocation location;
};

struct FunctionParameter {
  std::string    name;
  DataType       type;
  SourceLocation location;
};

struct FunctionEntry {
  std::string                    name; // unqualified function name
  std::shared_ptr<DataType>      return_type;
  std::vector<FunctionParameter> parameters;
  bool                           is_variadic = false;
  bool                           is_defined  = false; // has a body, vs. just declared
  bool                           is_used     = false;

  uint32_t       scope_id = 0; // scope the function body introduces
  SourceLocation location;
};

inline PrimitiveType
get_datatypes(const std::string_view keyword) {
  constexpr static std::pair<std::string, PrimitiveType> m_datatype_map[] = {
      {"int", PrimitiveType::INT},
      {"void", PrimitiveType::VOID},
      {"char", PrimitiveType::CHAR},
      {"float", PrimitiveType::FLOAT},
      {"bool", PrimitiveType::BOOL},
  };
  for (const auto& [view, data_type] : m_datatype_map) {
    if (view == keyword) {
      return data_type;
    }
  }
  return PrimitiveType::USER_DEFINED;
}
//
using DC_VALUE = uint16_t;
enum class DATATYPE_COMPATIBILITY : DC_VALUE {
  NONE           = 0,
  INT_LITERAL    = 1 << 0,
  FLOAT_LITERAL  = 1 << 1,
  HEX_LITERAL    = 1 << 2,
  BINARY_LITERAL = 1 << 3,
  STRING_LITERAL = 1 << 4,
  CHAR_LITERAL   = 1 << 5,
  BOOL_LITERAL   = 1 << 6,
};

inline DATATYPE_COMPATIBILITY
operator|(DATATYPE_COMPATIBILITY lhs,
          DATATYPE_COMPATIBILITY rhs) {
  return DATATYPE_COMPATIBILITY((DC_VALUE)lhs | (DC_VALUE)rhs);
}

inline DATATYPE_COMPATIBILITY
get_datatype_compatability(PrimitiveType data_types) {
  using DC = DATATYPE_COMPATIBILITY;
  switch (data_types) {
  case ag::PrimitiveType::INT: {
    return DC::INT_LITERAL | DC::BINARY_LITERAL | DC::HEX_LITERAL;
  }

  case PrimitiveType::CHAR: {
    return DC::CHAR_LITERAL;
  }

  case PrimitiveType::BOOL: {
    return DC::BOOL_LITERAL | DC::INT_LITERAL | DC::BINARY_LITERAL | DC::HEX_LITERAL;
  }

  default:
    return DATATYPE_COMPATIBILITY::NONE;
  }
}
inline bool
is_compatible_datatype(PrimitiveType type,
                       PrimitiveType pri_type) {
  auto is_compatible_for = get_datatype_compatability(type);
  auto token_2_datatype  = get_datatype_compatability(pri_type);
  return static_cast<DC_VALUE>(token_2_datatype) & static_cast<DC_VALUE>(is_compatible_for);
}

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

void
print_error(const std::string_view error,
            const Parser&          parser,
            const SourceLocation&  location);
} // namespace ag

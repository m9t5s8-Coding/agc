#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
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

  const StructMember*
  find_member(std::string_view member_name) const {
    for (const auto& member : members) {
      if (member.name == member_name)
        return &member;
    }
    return nullptr;
  }
};

// Translation-unit-wide registry of user-defined struct layouts.  Struct
// declarations are parsed before the code that uses them, so the registry is
// populated during parsing and consulted by both semantic analysis and code
// generation.
class StructTable {
public:
  bool
  declare(StructDefinition definition) {
    if (definition.name.empty() || m_structs.contains(definition.name))
      return false;

    m_structs.emplace(definition.name, std::move(definition));
    return true;
  }

  const StructDefinition*
  lookup(std::string_view name) const {
    auto it = m_structs.find(std::string(name));
    return it == m_structs.end() ? nullptr : &it->second;
  }

  void
  reset() {
    m_structs.clear();
  }

private:
  std::unordered_map<std::string, StructDefinition> m_structs;
};

[[nodiscard]]
StructTable&
get_struct_table();

// Build the pointer-sized value type used for a struct in expressions and
// stack slots.  Struct values are represented by the address of their
// storage; the definition itself carries the byte layout.
DataType
make_struct_type(std::string_view name);

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
  std::string                    label; // mangled assembly/IR label
  std::shared_ptr<DataType>      return_type;
  std::vector<FunctionParameter> parameters;
  bool                           is_variadic = false;
  bool                           is_defined  = false; // has a body, vs. just declared
  bool                           is_used     = false;

  uint32_t       scope_id = 0; // scope the function body introduces
  SourceLocation location;
};

// Size of a value when it is kept in a stack slot.  Strings are represented
// by the address of their data, so they take one machine word.
inline uint32_t
get_type_size(const DataType& type) {
  // Struct values are pointers to their storage, so a struct variable occupies
  // one machine word even though the pointee layout may be much larger.
  if (type.kind == TypeKind::POINTER || type.kind == TypeKind::STRUCT)
    return 8;

  if (type.size_in_bytes != 0)
    return static_cast<uint32_t>(type.size_in_bytes);

  switch (type.primitive_type) {
  case PrimitiveType::CHAR:
    return 1;
  case PrimitiveType::FLOAT:
    return 4;
  case PrimitiveType::DOUBLE:
    return 8;
  case PrimitiveType::INT:
  case PrimitiveType::STRING:
  case PrimitiveType::BOOL:
  case PrimitiveType::USER_DEFINED:
    return 8;
  default:
    return 8;
  }
}

inline PrimitiveType
get_datatypes(const std::string_view keyword) {
  constexpr static std::pair<std::string, PrimitiveType> m_datatype_map[] = {
      {"int", PrimitiveType::INT},
      {"void", PrimitiveType::VOID},
      {"char", PrimitiveType::CHAR},
      {"float", PrimitiveType::FLOAT},
      {"bool", PrimitiveType::BOOL},
      {"string", PrimitiveType::STRING},
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

  case PrimitiveType::STRING: {
    return DC::STRING_LITERAL;
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

inline bool
is_void_pointer_type(const DataType& type) {
  return type.kind == TypeKind::POINTER && type.pointee &&
         type.pointee->kind == TypeKind::PRIMITIVE &&
         type.pointee->primitive_type == PrimitiveType::VOID;
}

inline bool
is_compatible_type(const DataType& expected,
                   const DataType& actual) {
  if (expected.primitive_type == PrimitiveType::UNEXPECTED ||
      actual.primitive_type == PrimitiveType::UNEXPECTED)
    return false;

  // `void*` is the untyped result of the allocation built-ins.  It converts
  // to and from any pointer, and `string` is represented as a byte pointer.
  if (is_void_pointer_type(expected) || is_void_pointer_type(actual)) {
    const auto pointer_like = [](const DataType& type) {
      return type.kind == TypeKind::POINTER ||
             type.primitive_type == PrimitiveType::STRING;
    };
    return pointer_like(expected) && pointer_like(actual);
  }

  if (expected.kind == TypeKind::POINTER || actual.kind == TypeKind::POINTER) {
    if (expected.kind != TypeKind::POINTER || actual.kind != TypeKind::POINTER)
      return false;
    if (!expected.pointee || !actual.pointee)
      return !expected.pointee && !actual.pointee;
    return is_compatible_type(*expected.pointee, *actual.pointee);
  }

  if (expected.primitive_type == PrimitiveType::USER_DEFINED ||
      actual.primitive_type == PrimitiveType::USER_DEFINED) {
    if (expected.primitive_type != actual.primitive_type)
      return false;
    if (!expected.type_name.empty() && !actual.type_name.empty() &&
        expected.type_name != actual.type_name)
      return false;
    return true;
  }

  if (expected.primitive_type == actual.primitive_type)
    return true;

  return is_compatible_datatype(expected.primitive_type, actual.primitive_type);
}

// A stable, assembly-safe spelling for a source type.  Pointers are encoded
// recursively so `int*` and `int**` get distinct labels.
inline std::string
mangle_type_name(const DataType& type) {
  if (type.kind == TypeKind::POINTER && type.pointee)
    return mangle_type_name(*type.pointee) + "_ptr";

  if (!type.type_name.empty())
    return std::string(type.type_name);

  switch (type.primitive_type) {
  case PrimitiveType::VOID:
    return "void";
  case PrimitiveType::INT:
    return "int";
  case PrimitiveType::FLOAT:
    return "float";
  case PrimitiveType::DOUBLE:
    return "double";
  case PrimitiveType::CHAR:
    return "char";
  case PrimitiveType::STRING:
    return "string";
  case PrimitiveType::BOOL:
    return "bool";
  case PrimitiveType::USER_DEFINED:
    return "user_defined";
  case PrimitiveType::IDENTIFIER:
    return "identifier";
  case PrimitiveType::UNEXPECTED:
    return "unexpected";
  }
  return "unknown";
}

inline std::string
type_display_name(const DataType& type) {
  if (type.kind == TypeKind::POINTER && type.pointee)
    return type_display_name(*type.pointee) + "*";

  if (!type.type_name.empty())
    return std::string(type.type_name);

  return mangle_type_name(type);
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

inline std::string
generate_function_label(std::string_view                      function_name,
                        const std::vector<FunctionParameter>& parameters) {
  std::string param_str;
  for (size_t i = 0; i < parameters.size(); i++) {
    param_str += mangle_type_name(parameters[i].type);
    if (i < parameters.size() - 1) {
      param_str += "$";
    }
  }

  return std::format("{}${}", function_name, param_str);
}

class Parser;
class Expression;

// Convert a top-level initializer expression into a constant assembly/LLVM
// initializer.  Globals currently support literal integers, string literals,
// and unary +/- around those literals.
std::string
global_initializer(Expression* expression);

void
print_error(const std::string_view error,
            Parser&                parser);

void
print_error(const std::string_view error,
            Parser&                parser,
            const SourceLocation&  location);

std::optional<DataType>
parse_type(Parser& parser);

std::optional<std::string>
parse_operator_symbol(Parser& parser);

class LabelCounters;
class CodeGenContext;
class Add_Literals {
public:
  static size_t
  add_string_literals(std::string_view literal) {
    s_literals.push_back(literal);
    return s_literals.size() - 1;
  }

  static void
  add_to_instruction(LabelCounters& counter,
                     Parser&        parser);

  static bool
  find_literal(std::string_view literal) {
    return std::find(s_literals.begin(), s_literals.end(), literal) != s_literals.end();
  }

private:
  inline static std::vector<std::string_view> s_literals;
};

void
add_system_call_function(CodeGenContext&);

} // namespace ag

#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
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

  // Opaque generic parameter such as `T` in `Stack<T>`.  Erased generics are
  // compatible with any concrete type so existing code can be instantiated
  // implicitly until monomorphization exists.
  bool is_generic_parameter = false;

  // Reference types use `pointee` for the referenced type.  `is_const` above
  // belongs to the binding/pointee qualifier used by `let` and `mut self`;
  // this flag records whether the reference itself permits writes through it
  // (`&mut T`).  Keeping the two separate is what lets a `let` binding hold a
  // mutable reference: the binding cannot be rebound, but `ref = value` (and
  // the explicit `*ref = value` form) writes through it.
  bool is_mutable_reference = false;

  std::shared_ptr<DataType> pointee;

  std::optional<size_t> array_length; // nullopt = unsized, e.g. int[]

  // A map literal's pointer type.  Iterating it walks key/value pairs rather
  // than single elements.
  bool                      is_map       = false;
  std::shared_ptr<DataType> map_key_type;

  std::shared_ptr<DataType>              return_type;
  // Element types of a tuple such as `(int, string)`.  Values use the same
  // pointer-sized-slot representation as array literals.
  std::vector<std::shared_ptr<DataType>> tuple_elements;
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
  bool           is_public   = false;
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

// Named type aliases such as `type Meters = float;`.  Aliases are registered
// during parsing so later declarations can use them in type positions.
class TypeAliasTable {
public:
  bool
  declare(std::string name,
          DataType    type) {
    if (name.empty() || m_aliases.contains(name))
      return false;
    m_aliases.emplace(std::move(name), std::move(type));
    return true;
  }

  const DataType*
  lookup(std::string_view name) const {
    const auto alias = m_aliases.find(std::string(name));
    return alias == m_aliases.end() ? nullptr : &alias->second;
  }

  void
  reset() {
    m_aliases.clear();
  }

private:
  std::unordered_map<std::string, DataType> m_aliases;
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

// Translation-unit-wide import bindings.  Bindings are grouped by the module
// that wrote the import so a module can keep its own aliases without leaking
// them into unrelated scopes.  The empty scope is the file's top level.
class ModuleTable {
public:
  void
  declare_module(std::string path) {
    if (!path.empty())
      m_modules.insert(std::move(path));
  }

  bool
  has_module(std::string_view path) const {
    return m_modules.contains(std::string(path));
  }

  // A module can be split across several files.  `mod ag::String;` loads
  // `ag/String.ag`, but its declarations still belong to the `ag` namespace.
  // This maps the file-qualified spelling (`ag::String`) back to the namespace
  // that owns the declarations (`ag`), so `import ag::String { String }` works
  // as expected.
  void
  add_module_part(std::string path,
                  std::string module_path) {
    if (path.empty() || module_path.empty())
      return;
    m_module_parts[std::move(path)] = std::move(module_path);
  }

  const std::string*
  find_module_part(std::string_view path) const {
    const auto part_it = m_module_parts.find(std::string(path));
    return part_it == m_module_parts.end() ? nullptr : &part_it->second;
  }

  // A module file may be loaded by more than one importer (for example both
  // the user program and `lib/ag/io.ag` may write `mod ag::String;`).  Loading
  // the same file twice would re-declare every function and fail, so the file
  // is remembered here and parsed only once.
  bool
  is_module_file_loaded(std::string_view path) const {
    return m_loaded_module_files.contains(std::string(path));
  }

  void
  mark_module_file_loaded(std::string path) {
    if (!path.empty())
      m_loaded_module_files.insert(std::move(path));
  }

  void
  add_module_alias(std::string_view scope,
                   std::string      alias,
                   std::string      target) {
    if (alias.empty() || target.empty())
      return;
    m_scopes[std::string(scope)].module_aliases[std::move(alias)] = std::move(target);
  }

  void
  add_symbol_alias(std::string_view scope,
                   std::string      name,
                   std::string      target) {
    if (name.empty() || target.empty())
      return;
    m_scopes[std::string(scope)].symbol_aliases[std::move(name)] = std::move(target);
  }

  // Importing a module also makes that module's public operators available to
  // operator-overload resolution in the importing scope.
  void
  add_imported_module(std::string_view scope,
                      std::string      path) {
    if (path.empty())
      return;
    m_scopes[std::string(scope)].imported_modules.insert(std::move(path));
  }

  std::vector<std::string>
  imported_modules(std::string_view scope) const {
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;

    const auto append_scope = [&](std::string_view search_scope) {
      const auto scope_it = m_scopes.find(std::string(search_scope));
      if (scope_it == m_scopes.end())
        return;
      for (const auto& module : scope_it->second.imported_modules) {
        if (seen.insert(module).second)
          result.push_back(module);
      }
    };

    append_scope(scope);
    if (!scope.empty())
      append_scope({});
    return result;
  }

  const std::string*
  find_module_alias(std::string_view scope,
                    std::string_view alias) const {
    const auto scope_it = m_scopes.find(std::string(scope));
    if (scope_it == m_scopes.end())
      return nullptr;
    const auto alias_it = scope_it->second.module_aliases.find(std::string(alias));
    return alias_it == scope_it->second.module_aliases.end() ? nullptr : &alias_it->second;
  }

  const std::string*
  find_symbol_alias(std::string_view scope,
                    std::string_view name) const {
    const auto scope_it = m_scopes.find(std::string(scope));
    if (scope_it == m_scopes.end())
      return nullptr;
    const auto alias_it = scope_it->second.symbol_aliases.find(std::string(name));
    return alias_it == scope_it->second.symbol_aliases.end() ? nullptr : &alias_it->second;
  }

  void
  reset() {
    m_scopes.clear();
    m_modules.clear();
    m_module_parts.clear();
    m_loaded_module_files.clear();
  }

private:
  struct ScopeAliases {
    std::unordered_map<std::string, std::string> module_aliases;
    std::unordered_map<std::string, std::string> symbol_aliases;
    std::unordered_set<std::string>              imported_modules;
  };

  std::unordered_map<std::string, ScopeAliases> m_scopes;
  std::unordered_set<std::string>               m_modules;
  std::unordered_map<std::string, std::string>  m_module_parts;
  std::unordered_set<std::string>               m_loaded_module_files;
};

[[nodiscard]]
ModuleTable&
get_module_table();

[[nodiscard]]
StructTable&
get_struct_table();

TypeAliasTable&
get_type_alias_table();

// Build the pointer-sized value type used for a struct in expressions and
// stack slots.  Struct values are represented by the address of their
// storage; the definition itself carries the byte layout.
DataType
make_struct_type(std::string_view name);

struct EnumField {
  std::string    name;
  DataType       type;
  size_t         byte_offset = 0;
  SourceLocation location;
};

struct EnumVariant {
  std::string            name;
  std::vector<EnumField> fields;
  size_t                 index = 0;
  SourceLocation         location;

  const EnumField*
  find_field(std::string_view field_name) const {
    for (const auto& field : fields) {
      if (field.name == field_name)
        return &field;
    }
    return nullptr;
  }
};

// An enum value is represented by a pointer to heap storage.  The first
// machine word holds the variant index; payload fields start at byte offset 8
// and use the same pointer-sized slot layout as structs.
struct EnumDefinition {
  std::string               name;
  std::vector<EnumVariant>  variants;
  size_t                    size_in_bytes = 8;
  size_t                    alignment     = 8;
  SourceLocation            location;

  const EnumVariant*
  find_variant(std::string_view variant_name) const {
    for (const auto& variant : variants) {
      if (variant.name == variant_name)
        return &variant;
    }
    return nullptr;
  }
};

class EnumTable {
public:
  bool
  declare(EnumDefinition definition) {
    if (definition.name.empty() || m_enums.contains(definition.name))
      return false;

    m_enums.emplace(definition.name, std::move(definition));
    return true;
  }

  const EnumDefinition*
  lookup(std::string_view name) const {
    auto it = m_enums.find(std::string(name));
    return it == m_enums.end() ? nullptr : &it->second;
  }

  void
  reset() {
    m_enums.clear();
  }

private:
  std::unordered_map<std::string, EnumDefinition> m_enums;
};

[[nodiscard]]
EnumTable&
get_enum_table();

DataType
make_enum_type(std::string_view name);

// Resolve a source type name to either a struct or enum value type.  Used by
// `Self`, `self`, and impl blocks, where the kind is not known syntactically.
DataType
make_named_type(std::string_view name);

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

// The module part of a fully-qualified declaration name.  Top-level names
// have an empty module path.
inline std::string
declaring_module(std::string_view qualified_name) {
  const auto separator = qualified_name.rfind("::");
  if (separator == std::string_view::npos)
    return {};
  return std::string(qualified_name.substr(0, separator));
}

// Items are private by default.  A private declaration is visible from the
// module that declared it and from descendant modules; `pub` makes it visible
// from any module.  The top-level module is the root, so its private items are
// visible throughout the translation unit.
inline bool
is_module_or_ancestor(std::string_view ancestor, std::string_view module) {
  if (ancestor == module || ancestor.empty())
    return true;
  if (module.size() <= ancestor.size())
    return false;
  return module.compare(0, ancestor.size(), ancestor) == 0 &&
         module.substr(ancestor.size(), 2) == "::";
}

inline bool
is_visible_from(bool             is_public,
                std::string_view declaration_module,
                std::string_view current_module) {
  return is_public || is_module_or_ancestor(declaration_module, current_module);
}

class Expression;

struct FunctionParameter {
  std::string                 name; // empty for an unnamed parameter
  DataType                    type;
  std::shared_ptr<Expression> default_value; // evaluated at the call site
  SourceLocation              location;
};

struct FunctionEntry {
  std::string                    name; // unqualified function name
  std::string                    label; // mangled assembly/IR label
  std::string                    module_path;
  std::shared_ptr<DataType>      return_type;
  std::vector<FunctionParameter> parameters;
  bool                           is_variadic = false;
  bool                           is_defined  = false; // has a body, vs. just declared
  // Declared inside an `extern "c"` block.  External functions use their exact
  // C symbol as the IR label and may be called without an AGC definition; the
  // linker resolves them against a library or object file.
  bool                           is_extern   = false;
  bool                           is_used     = false;
  bool                           is_public   = false;
  // Associated member functions have no implicit receiver and are called as
  // `Type::name(...)`.  Module-level `static func` also records this flag, but
  // it is only meaningful for functions owned by an `impl`.
  bool                           is_static   = false;

  uint32_t       scope_id = 0; // scope the function body introduces
  SourceLocation location;
};

// Size of a value when it is kept in a stack slot.  Strings are represented
// by the address of their data, so they take one machine word.
inline uint32_t
get_type_size(const DataType& type) {
  // Struct values are pointers to their storage, so a struct variable occupies
  // one machine word even though the pointee layout may be much larger.
  if (type.kind == TypeKind::POINTER || type.kind == TypeKind::REFERENCE ||
      type.kind == TypeKind::STRUCT || type.kind == TypeKind::ENUM)
    return 8;

  if (type.size_in_bytes != 0)
    return static_cast<uint32_t>(type.size_in_bytes);

  switch (type.primitive_type) {
  case PrimitiveType::CHAR:
  case PrimitiveType::BOOL:
    return 1;
  case PrimitiveType::FLOAT:
    return 4;
  case PrimitiveType::DOUBLE:
    return 8;
  case PrimitiveType::INT:
  case PrimitiveType::STRING:
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
      {"byte", PrimitiveType::CHAR},
      {"float", PrimitiveType::FLOAT},
      {"double", PrimitiveType::DOUBLE},
      {"bool", PrimitiveType::BOOL},
      {"string", PrimitiveType::STRING},
      // Fixed-width integer spellings.  They all keep the existing signed
      // integer code-path (`PrimitiveType::INT`) but carry their storage width
      // and signedness in the DataType so memory accesses can use the right
      // machine type.  `int` itself remains a pointer-sized (8 byte) slot.
      {"uint", PrimitiveType::INT},
      {"u8", PrimitiveType::INT},
      {"i8", PrimitiveType::INT},
      {"u16", PrimitiveType::INT},
      {"i16", PrimitiveType::INT},
      {"u32", PrimitiveType::INT},
      {"i32", PrimitiveType::INT},
      {"u64", PrimitiveType::INT},
      {"i64", PrimitiveType::INT},
  };
  for (const auto& [view, data_type] : m_datatype_map) {
    if (view == keyword) {
      return data_type;
    }
  }
  return PrimitiveType::USER_DEFINED;
}

// Fill in the storage width, alignment, and signedness for a built-in type
// spelling.  `type.type_name` must already contain the spelling (or be empty
// for a plain `int`, which keeps its historical 8-byte representation).
inline void
apply_builtin_type_properties(DataType& type) {
  const std::string_view name = type.type_name;

  auto set = [&](size_t size, bool is_signed) {
    type.size_in_bytes = size;
    type.alignment     = size;
    type.is_signed     = is_signed;
  };

  if (name == "void") {
    set(0, false);
    return;
  }
  if (name == "bool") {
    type.primitive_type = PrimitiveType::BOOL;
    set(1, false);
    return;
  }
  if (name == "char" || name == "byte") {
    // `char`/`byte` keep the historical zero-extended byte behavior.
    set(1, false);
    return;
  }
  if (name == "i8") {
    set(1, true);
    return;
  }
  if (name == "u8") {
    set(1, false);
    return;
  }
  if (name == "i16") {
    set(2, true);
    return;
  }
  if (name == "u16") {
    set(2, false);
    return;
  }
  if (name == "uint" || name == "u32") {
    set(4, false);
    return;
  }
  if (name == "i32") {
    set(4, true);
    return;
  }
  if (name == "int" || name == "i64" || name == "u64" || name == "string") {
    set(8, name != "u64");
    return;
  }
  if (name == "float") {
    set(4, true);
    return;
  }
  if (name == "double") {
    set(8, true);
    return;
  }
}

// Convenience constructor used by the lexer/parser for literal types and by
// built-in type resolution.
inline DataType
make_builtin_type(std::string_view spelling) {
  DataType type;
  type.kind           = TypeKind::PRIMITIVE;
  type.type_name      = spelling;
  type.primitive_type = get_datatypes(spelling);
  apply_builtin_type_properties(type);
  return type;
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
  // `char` is an integer byte in AGC.  Treat it like the C family so byte
  // literals, indexes, and arithmetic can be mixed without casts.
  if ((type == PrimitiveType::CHAR && pri_type == PrimitiveType::INT) ||
      (type == PrimitiveType::INT && pri_type == PrimitiveType::CHAR))
    return true;

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

// A type that carries an address: raw pointers, references, and the legacy
// `string` spelling (a byte address).
inline bool
is_indirect_type(const DataType& type) {
  return type.kind == TypeKind::POINTER || type.kind == TypeKind::REFERENCE ||
         type.primitive_type == PrimitiveType::STRING;
}

// `char*` is the C-style byte pointer.  `string` remains as a compatibility
// spelling for the same pointer-sized byte address so existing code keeps
// working while new library code can use `char*`.
inline bool
is_char_pointer_type(const DataType& type) {
  return type.kind == TypeKind::POINTER && type.pointee &&
         type.pointee->kind == TypeKind::PRIMITIVE &&
         type.pointee->primitive_type == PrimitiveType::CHAR;
}

inline bool
is_string_like_type(const DataType& type) {
  return type.primitive_type == PrimitiveType::STRING || is_char_pointer_type(type);
}

inline bool
is_compatible_type(const DataType& expected,
                   const DataType& actual) {
  if (expected.primitive_type == PrimitiveType::UNEXPECTED ||
      actual.primitive_type == PrimitiveType::UNEXPECTED)
    return false;

  // Erased generic parameters accept any concrete type.
  if (expected.is_generic_parameter || actual.is_generic_parameter)
    return true;

  // `void*` is the untyped result of the allocation built-ins.  It converts
  // to and from any pointer, and `string` is represented as a byte pointer.
  if (is_void_pointer_type(expected) || is_void_pointer_type(actual)) {
    const auto pointer_like = [](const DataType& type) {
      return is_indirect_type(type);
    };
    return pointer_like(expected) && pointer_like(actual);
  }

  // String literals, `string`, and `char*` are all byte addresses and can be
  // assigned or passed to one another.
  if (is_string_like_type(expected) && is_string_like_type(actual))
    return true;

  const bool expected_indirect = expected.kind == TypeKind::POINTER ||
                                 expected.kind == TypeKind::REFERENCE;
  const bool actual_indirect = actual.kind == TypeKind::POINTER ||
                               actual.kind == TypeKind::REFERENCE;

  if (expected_indirect || actual_indirect) {
    if (!expected_indirect || !actual_indirect)
      return false;
    if (!expected.pointee || !actual.pointee)
      return !expected.pointee && !actual.pointee;

    // A `&T` may read through either a shared reference or a mutable one.
    // A `&mut T` needs mutable access; raw pointers are treated as capable of
    // mutation because their safety checking is deliberately outside the type
    // system for now.
    if (expected.kind == TypeKind::REFERENCE && expected.is_mutable_reference &&
        actual.kind == TypeKind::REFERENCE && !actual.is_mutable_reference)
      return false;

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

  if (type.kind == TypeKind::REFERENCE && type.pointee)
    return mangle_type_name(*type.pointee) +
           (type.is_mutable_reference ? "_mutref" : "_ref");

  if (!type.type_name.empty()) {
    // Module-qualified type names contain `::`, which is not valid in an
    // assembly label.  Encode the separator while keeping unqualified names
    // (the common case) byte-for-byte unchanged.
    std::string name;
    name.reserve(type.type_name.size());
    for (size_t i = 0; i < type.type_name.size(); ++i) {
      if (type.type_name[i] == ':' && i + 1 < type.type_name.size() &&
          type.type_name[i + 1] == ':') {
        name.push_back('$');
        ++i;
      } else {
        name.push_back(type.type_name[i]);
      }
    }
    return name;
  }

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

  if (type.kind == TypeKind::REFERENCE && type.pointee)
    return std::string(type.is_mutable_reference ? "&mut " : "&") +
           type_display_name(*type.pointee);

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

// Parse a generic parameter list such as `<T>`, `<T, U>` or
// `<T, const N: int>`.  The parameters are not monomorphized yet; only their
// names are returned so type positions can treat them as opaque types.
std::optional<std::vector<std::string>>
parse_generic_parameter_list(Parser& parser);

// Consume a generic argument list such as `<int, string>` after a type name.
bool
skip_generic_arguments(Parser& parser);

// Consume a `where T: Bound + Other` clause.  Bounds are accepted and
// ignored until trait dispatch exists.
bool
skip_where_clause(Parser& parser);

std::optional<std::string>
parse_operator_symbol(Parser& parser);

class LabelCounters;
class CodeGenContext;

// `static` locals keep their value between calls.  Emit a hidden module-level
// slot for the value and, for non-constant initializers, a guard slot that
// makes the initializer run exactly once.
void
generate_static_local(LabelCounters&         counter,
                      Parser&                parser,
                      std::string_view       name,
                      const DataType&        type,
                      Expression*            initializer,
                      bool                   is_const);

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

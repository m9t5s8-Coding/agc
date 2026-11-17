# 02 — Types

AGC's type system is small. Most values occupy one 8-byte machine slot; `char`,
`bool`, `float`, and the fixed-width integers use their declared storage width
in memory. Structs, enums, arrays, maps, and strings are represented by pointers
to storage.

---

## 02.1 Primitive types

| Type | Size | Meaning |
| --- | --- | --- |
| `int` | 8 bytes | Default signed integer. Unsuffixed integer literals infer `int`. |
| `uint` | 4 bytes | Unsigned 32-bit integer. Literal suffix `u`. |
| `u8` / `i8` | 1 byte | Unsigned / signed 8-bit integer. |
| `u16` / `i16` | 2 bytes | Unsigned / signed 16-bit integer. |
| `u32` / `i32` | 4 bytes | Unsigned / signed 32-bit integer. |
| `u64` / `i64` | 8 bytes | Unsigned / signed 64-bit integer. |
| `char` | 1 byte | Byte / character (zero-extended when read). |
| `byte` | 1 byte | Alias for `char`. |
| `float` | 4 bytes | IEEE-754 single precision. |
| `double` | 8 bytes | Recognised for FFI signatures; source-level conversions are not implemented yet. |
| `bool` | 1 byte | Boolean. `true` and `false` have type `bool`; comparisons still yield `int` `0`/`1`. |
| `string` | 8 bytes | `char*` to a NUL-terminated byte buffer. |
| `void` | — | “No value”; valid as a return type. |

```ag
let count: int = 10;
let small: u8 = 255u8;
let signed: i8 = -5i8;
let word: u16 = 60000u16;
let flags: u32 = 4000000000u32;
let wide: u64 = 123456789u64;
let letter: char = 'A';
let raw: byte = 0x41;       # byte is char
let ratio: float = 0.5;
let ready: bool = true;
let name: string = "ag";
```

A decimal literal without a suffix is an `int`; `false`/`true` produce a
`bool`; and `0u` produces a `uint`:

```ag
let x = 0;       # int
let b = false;   # bool
let u = 0u;      # uint
```

> **Note.** `bool` has a 1-byte storage representation but is still tested as
> “nonzero vs. zero” in conditions. Comparisons (`==`, `<`, `!=`, ...) yield an
> `int` `0`/`1`, even when their operands are `bool` or a sized integer.

### Sized-integer semantics

Locals are held in 8-byte registers/slots, and arithmetic is width-aware. Each
operation uses the **common type** of its operands (shifts use the left
operand's type), and the result is wrapped back to that width before it returns
to the slot. Assignment, argument passing, returning, and memory access also
narrow to the declared width. This gives the expected wrap-around behavior:

```ag
var a: u8 = 250u8;
a + 10u8;          # 4 — wraps at 8 bits inside the expression
a = a + 10;        # 4 — assignment narrows the 64-bit result

var b: i8 = 127i8;
b + 1i8;           # -128
b = b + 1;         # -128
```

Mixed-width operations widen to the wider operand, and `bool` promotes to
`int`:

```ag
var small: u8 = 200u8;
small + 100;       # 300 — `100` is `int`, so the common type is 64-bit
small + 100u8;     # 44  — common type is `u8`
```

Integer literals accept their full declared range. A `u64` literal above
`i64` max is stored as its two's-complement bit pattern:

```ag
var top: u64 = 18446744073709551615u64;   # u64 max
top >> 1u64;                              # 9223372036854775807
```

Limitations:

- `>>` is logical when the left operand is unsigned and arithmetic otherwise.
  `/`, `%`, `<`, `<=`, `>`, and `>=` use unsigned instructions when the
  **common type** is unsigned. A narrow unsigned value mixed with a wider
  signed value therefore uses signed semantics.
- `sizeof` does not exist yet.

There is no `long` or `short` keyword; use the sized spellings above. An unknown
bare name is treated as a user-defined type.

A runnable proof lives in `examples/sized_ints/main.ag`.

---

## 02.2 Pointers

`T*` is a raw pointer to `T`. Multiple stars are allowed (`T**`).

```ag
var buffer: char* = alloc(16);   # void* converts implicitly to char*
buffer[0] = 'h';
buffer[1] = 'i';
buffer[2] = 0;

var numbers: int* = alloc(24);   # room for three int slots
numbers[0] = 10;
numbers[1] = 20;
numbers[2] = 30;

free(numbers);
free(buffer);
```

- `alloc`/`malloc` return `void*` and convert implicitly to any object pointer.
- Pointers support indexing `p[i]`, dereference `*p`, and addition `p + n`.
- Pointer subtraction (`p - q`) is **not** implemented (see chapter 11).
- `char*` and `string` are interchangeable in most positions.

> **Runs.** Pointers, indexing, dereference, and pointer addition work.

---

## 02.3 References

A reference is a transparent alias to an existing value. It is written `&T`
(shared) or `&mut T` (mutable).

```ag
func bump(value: &mut int) {
    value = value + 1;      # writes through the reference
}

func read(value: &int) -> int => value;

func main() -> int {
    var x: int = 5;
    bump(&mut x);
    let y = read(&x);
    return y - 6;
}
```

References are transparent: you normally use the name directly, not `*ref`.
Explicit `*ref` is also accepted for reads and writes.

```ag
var x = 1;
let r: &mut int = &mut x;
*r = 2;       # same effect as `r = 2`
let v = *r;   # same effect as `r`
```

Rules:

- `&value` creates a shared reference; `&mut value` creates a mutable one.
- Assigning through `&mut T` is allowed even when the binding itself is `let`.
- Assigning through `&T` is rejected.
- A reference may point at a local, a parameter, a field, or an element.

> **Runs.** References are implemented for parameters and locals. There is no
> lifetime checker; use them carefully.

---

## 02.4 Arrays and slices

Array types are written `T[]` (unsized) or `T[N]` (statically known length).

```ag
let unsized: int[] = [1, 2, 3];
let fixed: int[3] = [1, 2, 3];
let bytes: char[4] = ['a', 'b', 'c', '\0'];
```

An array uses the same pointer representation as `T*`; `array_length` is stored
in the type so `for` can iterate it.

```ag
let values: int[4] = [10, 20, 30, 40];

let first = values[0];
let last  = values[3];

for i in 0..4 {
    println(values[i]);         # assumes ag::io is imported
}

for i, value in values {     # index plus value
    # ...
}
```

### Array literals

`[a, b, c]` allocates a block on the heap and returns a pointer. Elements are
packed using their inferred storage width (`bool`/`u8`/`i8` use one byte,
`u16`/`i16` two, `u32`/`uint` four, and eight-byte types eight). The literal's
inferred type remembers its length, so it can be iterated directly.

```ag
for value in [1, 2, 3] {
    println(value);             # assumes ag::io is imported
}
```

An empty array literal `[]` has type `void*`; it is mainly useful as a generic
placeholder.

### Slices

Range indexing parses but is **not** a view:

```ag
let part = values[1..3];   # parses
let x = part[0];           # currently reads the original storage at index 0
```

> **Parse-only / incorrect result.** Slicing does not adjust the start or end;
> the expression behaves as the original pointer. Pass an offset pointer
> explicitly if you need that behavior.

---

## 02.5 Tuples

Tuple types are written `(T1, T2, ...)`. Tuple values use one 8-byte slot per
element.

```ag
func pair() -> (int, string) => (1, "one");

func main() -> int {
    let t: (int, string) = pair();
    let (number, text) = t;      # destructure
    let also: (int, int) = (3, 4);
    return number - 1;
}
```

There is **no tuple indexing** (`t.0` or `t[0]`) and no named tuple field
access. Destructure the tuple into names instead.

> **Runs.** Construction, storage, assignment, and `let (a, b) = ...`
> destructuring work. Direct element access is missing.

---

## 02.6 Optional types

A `?` suffix marks a type as optional:

```ag
func find() -> int? => 0;
let maybe: int? = find();
```

The current compiler accepts the syntax but keeps the underlying
representation. `T?` is effectively `T` at runtime:

- There is no dedicated `Option<T>` type.
- `null` / `nullptr` is a typed pointer literal, not an optional value.
- Postfix `?` and `!` are accepted but are no-ops.
- `orelse` / `??` test the value against zero.

```ag
let value: int? = 5;
let a = value orelse 9;   # 5
let b = 0 ?? 9;           # 9
```

> **Parse-only / no-op.** Optional typing and postfix `?`/`!` do not perform
> real option unwrapping. See chapter 12.

---

## 02.7 Function types

A function type is written `func(ParamTypes...) -> ReturnType`.

```ag
type Callback = func(int) -> int;
type Predicate<T> = func(T) -> bool;
```

Function types parse and can appear in aliases, fields, and parameters, but
first-class function **values** are not implemented. You cannot yet assign a
function name to a variable or call through a function-typed value.

```ag
func twice(value: int) -> int => value * 2;

let fn: Callback = twice;   # error: undefined variable `twice`
```

> **Parse-only.** Function types are opaque callable-shaped values with no
> runtime support. Call functions directly by name.

---

## 02.8 User-defined types

Struct and enum names are types. They may be module-qualified.

```ag
mod shapes {
    pub struct Point { pub x: int, pub y: int }
    pub enum Status { Ok, Failed(string) }
}

let p: shapes::Point = shapes::Point { x: 1, y: 2 };
let s: shapes::Status = shapes::Status::Ok;
```

Inside an `impl`, `Self` refers to the type being implemented.

```ag
struct Counter { value: int }

impl Counter {
    func make() -> Self => Counter { value: 0 };
    func get(self) -> int => self.value;
}
```

A struct or enum value is represented by a pointer to its heap storage, so a
variable of struct type occupies one machine word.

> **Runs.** See chapter 08 for layout, literals, and methods.

---

## 02.9 Type aliases

`type Name = Type;` creates an alias. Aliases are resolved during type parsing.

```ag
type Meters = float;
type Handle = void*;
type Pair = (int, int);

struct Box {
    width: Meters,
    content: Pair,
}
```

Generic aliases are accepted syntactically but not instantiated:

```ag
type Predicate<T> = func(T) -> bool;   # parse-only
```

Aliases do not create a distinct type; `Meters` and `float` are interchangeable.

---

## 02.10 Generic types

Generic arguments are written with angle brackets.

```ag
struct Box<T> {
    value: T,
}

impl<T> Box<T> {
    func get(self) -> T => self.value;
}

struct Ring<T, const N: int> {
    data: T[N],
    head: int,
}
```

Generic parameters are erased:

- `T` is an opaque pointer-sized value.
- `Box<int>` and `Box<string>` share the same implementation.
- `const N: int` is parsed but not evaluated; `T[N]` is accepted as an array
  type without a resolved length.
- Explicit arguments such as `Box<int>` and `add<int>(1, 2)` are skipped by the
  parser; they do not change code generation.

```ag
func max<T>(a: T, b: T) -> T => a > b ? a : b;

let m = max(3, 9);          # works
let n = max<int>(3, 9);     # explicit argument is erased
```

> **Runs (erased).** Generic syntax and basic operations work. There is no
> monomorphization, specialization, or trait dispatch. See chapter 10.

---

## 02.11 Variadic parameter types

A trailing `...` on a parameter type means “zero or more values of this type”.

```ag
func total(values: int...) -> int {
    var result = 0;
    for value in values {
        result += value;
    }
    return result;
}

total();
total(1);
total(1, 2, 3);
```

At the call site, values are packed into a block whose first slot holds the
count; the parameter points at the first element.

```ag
let values: int[3] = [1, 2, 3];
total(...values);   # spread an existing array
```

> **Runs.** Variadic parameters work with direct arguments and `...array`
> spreads.

---

## 02.12 Inferred map types

There is no `map<K, V>` type spelling. A map literal produces a pointer type
that records its key and value types:

```ag
let scores = map { "ann": 90, "bob": 75 };
for name, score in scores {
    # name: string, score: int
}
```

The type is only produced by `map { ... }`; you cannot write it directly in a
declaration. See chapter 13 for map iteration and layout.

---

## 02.13 Type compatibility rules

The analyzer is permissive:

- Integer literals are compatible with `int`, `char`, `bool`, and the sized
  integer types in many contexts.
- `char` and `int` are interchangeable in arithmetic.
- The fixed-width integer types are compatible with `int` for arithmetic;
  assignment, argument passing, and return narrow to the declared width.
- `string` literals are compatible with `string` and `char*`.
- `void*` converts implicitly to any pointer type.
- `null` and `nullptr` have type `void*`; they convert to any pointer type but
  not to an integer. A reference binding also accepts the literal, but a null
  reference must not be read or compared with `null`.
- A generic parameter `T` is compatible with every concrete type.

```ag
let p: char* = "ok";       # string literal to char*
let q: int* = alloc(8);    # void* to int*
let z: int* = nullptr;     # null pointer
let c: int = 'A';          # char value in an int slot
let x: T;                  # only inside generic code
# let bad: int = nullptr;  # error: Incompatible types
```

Explicit casts use `as`:

```ag
let wide: int = 300;
let low: char = wide as char;   # keeps the low 8 bits (44)
let back: int = low as int;     # 44
```

> **Runs, but casts are narrow.** Only `as char` actually changes the value (it
> masks to 8 bits). Other `as` casts currently pass the representation through;
> they are not numeric conversions. See chapter 04.

---

## 02.14 Quick reference

```ag
int                     # 8-byte signed integer
uint                    # 4-byte unsigned integer
u8 i8                   # 1-byte integers
u16 i16                 # 2-byte integers
u32 i32                 # 4-byte integers
u64 i64                 # 8-byte integers
char  byte              # 1-byte byte
float                   # 4-byte float
double                  # 8-byte float (FFI signatures)
bool                    # 1-byte boolean
string                  # char*
void

T*                      # raw pointer
T**                     # pointer to pointer
&T     &mut T           # shared / mutable reference

T[]    T[N]             # array (pointer-shaped)
(A, B)                  # tuple
T?                      # optional (parse-only suffix)
func(A, B) -> C         # function type (parse-only values)
Name<T, U>              # generic user type (erased)

type Alias = T;         # type alias
```

Next: [03 — Declarations](03-declarations.md).

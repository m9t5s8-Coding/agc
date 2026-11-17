# 15 — Attributes, compile-time markers, and missing features

This chapter collects the smaller declaration forms that do not fit elsewhere,
then gives an honest inventory of what the current compiler does **not** support.

---

## 15.1 Attributes: `@name`

Attributes are written above a declaration. They are parsed and ignored.

```ag
@inline
func clamp(value: int, low: int, high: int) -> int =>
    value < low ? low : value > high ? high : value;
```

An attribute may take one parenthesised argument list:

```ag
@deprecated("use add instead")
func old_add(a: int, b: int) -> int => a + b;
```

Attributes may be stacked, and they may decorate any statement the parser
accepts, not only functions:

```ag
@inline
@deprecated("legacy")
let GLOBAL_LIMIT: int = 3;
```

There is no validation, no effect on code generation, and no warning output.

> **Parse-only (ignored).** `@inline` does not force inlining; `@deprecated`
> does not warn. The names themselves are not checked.

---

## 15.2 `comptime`

`comptime` is a marker before a declaration or an expression.

### On a declaration

```ag
comptime func fib(n: int) -> int =>
    n < 2 ? n : fib(n - 1) + fib(n - 2);
```

The function is an ordinary function. There is no compile-time function
evaluation.

### On an expression

```ag
let x = comptime 5;          # evaluates to 5 at run time
```

The expression is evaluated normally.

```ag
const TABLE: int = comptime fib(10);
```

This compiles, but see chapter 03: module-level initialisers are limited to
literals. `TABLE` is therefore `0`, not `55`.

> **No-op (marker).** `comptime` does not evaluate anything early.

---

## 15.3 `extern "c"` blocks

`extern "c" { ... }` declares functions supplied by a linked C library, object
file, or static archive. They look like C prototypes and need no AGC body.

```ag
extern "c" {
    func strlen(text: char*) -> int;
    func puts(text: char*) -> int;
}

func main() -> int {
    puts("hello from libc");
    return strlen("hello") - 5;
}
```

```sh
build/agc libc.ag -o /tmp/libc      # libc is linked by clang automatically
/tmp/libc
```

External declarations keep their exact C symbol names: no module prefix and no
AGC type suffix. A call is resolved by the system linker when the executable is
built. Link additional libraries with `-L <dir>`, `-l <name>`, or
`--link-arg <arg>`:

```sh
build/agc app.ag -L /opt/mylib/lib -l mylib \
    --link-arg -Wl,-rpath,/opt/mylib/lib -o /tmp/app
build/agc app.ag --link-arg /opt/mylib/lib/libmylib.a -o /tmp/app
```

The common scalar mappings are: pointer-like AGC types pass as `ptr`; plain
`int` maps to C `i32` (truncated from the AGC i64 slot on arguments,
sign-extended on returns); `u8`/`i8`/`char`/`bool` map to `i8`,
`u16`/`i16` to `i16`, `u32`/`i32`/`uint` to `i32`, and `u64`/`i64` to
`i64`; unsigned narrow returns are zero-extended and signed ones are
sign-extended. `float` converts to and from a C `float`. Structs
by value, C varargs, callbacks/function pointers, and runtime `dlopen` loading
are not supported. This is Unix-only; Windows DLLs are not supported.

A prototype followed by an ordinary AGC definition still resolves normally:

```ag
func twice(value: int) -> int;

func twice(value: int) -> int => value * 2;

let n = twice(5);                   # 10
```

> **Runs (Unix link-time).** Calls resolve against libc, archives, and shared
> libraries at link time. The compiler targets Linux.

---

## 15.4 `trait` declarations

A trait may be declared before an `impl Trait for Type` block:

```ag
trait Shape {
    func area(self) -> int;
}

struct Square { side: int }

impl Shape for Square {
    func area(self) -> int => self.side * self.side;
}
```

The trait body is consumed by the parser and discarded; there is no trait object,
no dynamic dispatch, and no bound checking. The concrete implementation inside
`impl Shape for Square` is an ordinary method and is callable:

```ag
let square = Square { side: 3 };
println(square.area());         # 9
```

Generic trait spellings are also accepted syntactically:

```ag
trait Container<T> {
    func size(self) -> int;
}

impl Container<int> for Square {
    func size(self) -> int => self.side;
}
```

> **Parse-only (traits).** Trait declarations and impl headers parse; only the
> concrete methods have behavior. Bounds such as `T: Shape` are ignored.
> See chapter 08 for the `impl` rules.

---

## 15.5 `test` blocks and `assert`

`test` may be followed by an optional string name and a block:

```ag
test "addition works" {
    assert(1 + 1 == 2);
}

test {
    assert(1 == 1);
}
```

Test blocks are parsed and then **compiled out** of normal executables.

```sh
build/agc misc.ag -I lib -o /tmp/misc
/tmp/misc
```

The output does not include test results. There is no `agc test` mode and no test
runner.

`assert` only appears in test bodies in the examples. It is not a built-in
function; outside a `test` block it fails semantic analysis:

```ag
func main() -> int {
    assert(1 == 1);     # error: Undefined function
    return 0;
}
```

> **Parse-only.** Tests are discarded. `assert` is not resolved or executed.

---

## 15.6 `static func` and other modifiers

`static` before `func` is accepted as an explicit spelling of an associated
function (a function without `self`):

```ag
struct Math {
    value: int,
}

impl Math {
    static func square(n: int) -> int => n * n;
}

let n = Math::square(4);        # 16
```

`async` and `comptime` may also prefix `func`; both are ignored. `pub` / `priv`
are covered in chapters 05 and 09.

> **Runs (static).** `static func` behaves like an associated function.

---

## 15.7 `box`

`box value` is a prefix marker intended to request heap storage. It is currently
erased and evaluates its operand normally:

```ag
let point = box Point { x: 1, y: 2 };
println(point.x);               # 1
```

> **No-op (marker).** `box` does not allocate a wrapper or change lifetime.

---

## 15.8 Missing features

The following spellings are **not** implemented by the current compiler. The
examples show what happens if you try them.

### No `Option` / `Result` types

```ag
let value: Option<int> = 1;     # error: Incompatible types
let result: Result<int, int> = 0;
```

There are no built-in `Some`, `None`, `Ok`, or `Err` constructors. `Some(v)` and
`Ok(v)` are only accepted as **pattern** spellings (chapter 12) or if you declare
your own enum with those variant names.

### `null` is only a pointer literal

```ag
let p: int* = nullptr;          # valid: pointer to nothing
let q = null;                   # valid: inferred void*
```

`null` and `nullptr` are aliases. They have type `void*`, convert to any
pointer type, and can be passed to pointer parameters:

```ag
func takes(ptr: char*) -> int => ptr == nullptr ? 1 : 0;

let ok = takes(nullptr);        # 1
```

Reference bindings and parameters also accept the literal at compile time, but
they are not nullable: comparing a reference with `null` is rejected and
reading through a null reference crashes. Use a pointer for nullable values.

They are not an optional/`Option` value and do not convert to integers:

```ag
let n: int = nullptr;           # error: Incompatible types
```

A complete runnable demo is `examples/nullptr/main.ag`:

```sh
build/agc examples/nullptr/main.ag -o /tmp/ag_nullptr
/tmp/ag_nullptr
```

### No tuple indexing

```ag
let t = (1, 2);
let first = t[0];               # error: Invalid operand types
```

Destructure instead:

```ag
let (first, second) = (1, 2);
```

### No `sizeof`

```ag
let n = sizeof(int);            # error: Expected: expression
```

Allocate an explicit byte count. Struct fields use natural size and alignment
(capped at 8 bytes); primitive sizes are listed in chapter 02.

### No first-class functions or closures

Lambdas parse but do not create callable values:

```ag
let f = (n: int) -> int => n + 1;   # parses
let x = f(1);                        # error: Undefined function
```

A bare function name also fails when used as a value:

```ag
let callback = twice;           # error: Use of undefined variable
```

### No generic monomorphization

Generic parameters are erased (chapter 10). There is no separate specialization
for `Box<int>` and `Box<string>`, and generic enums do not parse:

```ag
enum Maybe<T> {                   # error: Expected: '{'
    None,
    Some(T),
}
```

### No optional chaining

`value?.field` parses as the no-op postfix `?` followed by `.field`. It does not
short-circuit, because there is no optional wrapper:

```ag
let x = point?.x;               # same as point.x
```

If the underlying pointer is `0`, this still dereferences it.

### No pointer subtraction

```ag
let offset = a - b;             # only meaningful for ordinary integers
```

### No slicing views

`values[1..3]` parses but does not adjust the base pointer or length
(chapter 11).

### No multiple declarators per statement

```ag
let a = 1, b = 2;               # error: Unexpected token
```

Write one declaration per statement.

### No wildcard imports

```ag
import ag::io::*;               # not supported
```

Import the module alias or list the names explicitly.

### No `try` / `catch` / exceptions

There is no exception machinery. Model failures with sentinel values,
out-parameters, or your own enums (chapter 12).

### No compile-time evaluation

`comptime` is a marker only (15.2).

### No Windows DLLs or runtime library loading

`extern "c"` is link-time only and Unix-only. There is no `LoadLibrary` /
`GetProcAddress` equivalent, and no `dlopen` / `dlsym` runtime lookup. Loading a
library whose name is only known at run time would need function-pointer values
and indirect calls, which the compiler does not have.

### No threading or synchronization library

`ag::sync`, `ag::thread`, `Mutex`, and `Channel` do not exist. `spawn`, `async`,
`await`, and `<-` are synchronous/pass-through markers (chapter 14).

### No string content comparison on plain `char*`

```ag
let same = "a" == "a";          # 0: compares addresses
```

Use `ag::String` when you need value comparison (chapter 13).

### No bounds checks, definite-assignment checks, or lifetime checks

Uninitialised reads, out-of-range indexes, use-after-free, and dangling
references are not diagnosed.

---

## 15.9 Status matrix

| Area | Status | Notes |
| --- | --- | --- |
| `let` / `var` / `const` / `static` | **Runs** | Module-level non-literal initialisers become `0`. |
| Functions, defaults, variadics, overloads | **Runs** | Ambiguity possible with erased generics. |
| Methods, associated functions, operators | **Runs** | `with` cannot be a method name. |
| `if` / `else` / `while` / `until` / `for` | **Runs** | `if`/`while`/`until` need parentheses; `for` does not. |
| `loop` expressions, labels, `guard` | **Runs** | `break value` yields an integer-shaped result. |
| `match`, `if let`, `while let` | **Runs** | No exhaustiveness checking; duplicate pattern names can collide. |
| Structs, enums, impls | **Runs** | Natural field size/alignment (max 8); aggregates are pointer-backed. |
| `uint`, sized integers, `bool` | **Runs** | Narrowed on assignment, arguments, returns, and memory access. Arithmetic uses the operands' common width and wraps there; unsigned `/`, `%`, `>>`, and ordered comparisons follow the common type's signedness (see 02.1). |
| Traits | **Parse-only** | Only concrete methods behave. |
| Modules, imports, visibility | **Runs** | Import/module visibility itself is not enforced. |
| Generics | **Runs (erased)** | No monomorphization; generic enums missing. |
| `where` bounds, generic aliases | **Parse-only** | Accepted and ignored. |
| Pointers, references, arrays | **Runs** | No bounds/lifetime checks; slices are not real views. |
| `null` / `nullptr` | **Runs** | Typed as `void*`; converts to pointer types. |
| `T?`, postfix `?` / `!` | **No-op** | Storage and behavior are unchanged. |
| `orelse`, `??`, `??=` | **Runs** | Truthiness is “non-zero”; no optional wrapper. |
| `ag::String`, `ag::io`, f-strings | **Runs** | `^` alignment and `b`/`o`/`d` fall back to decimal. |
| Attributes, `comptime`, `box` | **No-op** | Parsed and ignored. |
| `extern "c"` | **Runs (Unix link-time)** | Exact C symbol names; link with `-L`/`-l`/`--link-arg`. No structs by value, C varargs, callbacks, or DLLs. |
| `test` blocks | **Parse-only** | Compiled out; `assert` unresolved. |
| `spawn`, `async`, `await`, `<-` | **No-op** | Synchronous/pass-through. |
| `@`-attributes on any statement | **Parse-only** | No validation. |
| `Option` / `Result` | **Missing** | Declare your own enums; use `null`/`nullptr` only for pointers. |
| First-class functions / closures | **Missing** | Lambdas parse but cannot be called. |
| `sizeof`, tuple indexing, optional chaining | **Missing** | See 15.8. |
| Threading / synchronization | **Missing** | No runtime. |

---

## 15.10 Complete runnable example

```ag
# misc.ag
mod ag::io;
import ag::io { println };

@inline
func clamp(value: int, low: int, high: int) -> int =>
    value < low ? low : value > high ? high : value;

@deprecated("use clamp instead")
func legacy(value: int) -> int => value;

comptime func doubled(n: int) -> int => n * 2;

extern "c" {
    func strlen(text: char*) -> int;
}

trait Shape {
    func area(self) -> int;
}

struct Square {
    side: int,
}

impl Shape for Square {
    func area(self) -> int => self.side * self.side;
}

test "addition works" {
    assert(1 + 1 == 2);
}

func main() -> int {
    println(clamp(5, 1, 3));            # 3
    println(legacy(1));                 # 1
    println(doubled(2));                # 4
    println(strlen("hello"));           # 5, from libc

    let square = Square { side: 3 };
    println(square.area());             # 9

    let boxed = box Square { side: 4 };
    println(boxed.side);                # 4
    return 0;
}
```

```sh
build/agc misc.ag -I lib -o /tmp/misc
/tmp/misc
```

Next: [16 — Cheat sheet and grammar](16-cheatsheet.md).

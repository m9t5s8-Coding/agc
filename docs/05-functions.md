# 05 — Functions and operator overloads

Functions are declared with `func`. Methods are functions declared inside an
`impl` block. Operators are declared with `operator`.

---

## 05.1 Basic function declaration

```ag
func name(parameters) -> ReturnType {
    # body
}
```

The return type is optional. If it is omitted, the function returns `void`.

```ag
func greet() {
    println("hello");
}

func answer() -> int {
    return 42;
}

func add(a: int, b: int) -> int {
    return a + b;
}
```

A function returning a value must use `return` (or an expression body, below).

```ag
func bad() -> int {
    let x = 1;      # error: missing return
}
```

> **Runs.** Ordinary declarations, typed and untyped returns, and `void`
> functions are implemented.

---

## 05.2 Expression-bodied functions

`=> expression;` replaces a one-expression body. The expression is returned
when the function has a value return type, and evaluated for its side effects
when the return type is `void`.

```ag
func square(value: int) -> int => value * value;

func max(a: int, b: int) -> int => a > b ? a : b;

func announce(value: int) => println(value);   # void; println is the body

func set_size(mut self, n: int) => self.size = n;   # setter idiom
```

Both forms can coexist in one file. There is no `;` after the closing `}` of a
block body, but there **is** a `;` after an expression body.

> **Runs.** `=>` is fully supported, including `void` expression bodies.

---

## 05.3 Parameters

The full parameter spelling is `mut? name: Type`, with an optional default:

```ag
func f(a: int, b: int, c: int) { ... }
```

Detailed rules:

- Parameters are immutable by default.
- `mut name: Type` makes the parameter a mutable local copy.
- Parameters may be unnamed: `func f(int, int)`. Unnamed parameters can still
  be passed arguments, but the body cannot refer to them.
- A default value may be attached with `= expression`.
- Defaults must be trailing: once one parameter has a default, every later
  parameter must have one too.
- A variadic parameter must be last (see 05.5).

```ag
func offset(value: int, by: int = 1) -> int => value + by;

offset(10);        # by = 1
offset(10, 4);     # by = 4
```

```ag
func bump(mut value: int) -> int {
    value += 1;     # only the local copy changes
    return value;
}
```

> **Runs.** Immutable/mutable parameters, unnamed parameters, and trailing
> defaults all work.

### Mutating the caller's value

Passing by value copies the slot. To let the callee write to the caller's
storage, pass a pointer or a `&mut` reference:

```ag
func bump(value: &mut int) {
    value += 1;
}

var x = 1;
bump(&mut x);
# x is now 2
```

---

## 05.4 Return types

```ag
func nothing() { ... }              # void
func nothing2() -> void { ... }     # void, explicit
func number() -> int { ... }
func text() -> string { ... }
func ptr() -> int* { ... }
func pair() -> (int, string) { ... }
func maybe() -> int? { ... }        # optional suffix (parse-only semantics)
func generic<T>() -> T { ... }      # erased generic
```

`return;` is allowed in `void` functions. `return expression;` is required for
value-returning functions.

```ag
func choose(flag: bool) -> int {
    if (flag) {
        return 1;
    }
    return 0;
}
```

`return` may appear in expressions in the specific `orelse`/`??` fallback
position:

```ag
func required(value: int?) -> int {
    let n = value orelse return 0;
    return n;
}
```

---

## 05.5 Variadic parameters

A trailing `...` on the type makes the function variadic:

```ag
func total(values: int...) -> int {
    var sum = 0;
    for value in values {
        sum += value;
    }
    return sum;
}

total();
total(1);
total(1, 2, 3);
```

At the call site, the arguments are packed into a block whose type records the
element count. Inside the function, `values` behaves like an iterable array of
`int`.

Spread an existing array with `...`:

```ag
let values: int[3] = [1, 2, 3];
total(...values);
```

> **Runs.** Variadics and `...array` spreads work.

---

## 05.6 Named arguments

```ag
func offset(value: int, by: int) -> int => value + by;

offset(5, 2);                 # positional
offset(by: 2, value: 5);      # named, reordered
offset(5, by: 2);             # mixed
```

Rules and limitations:

- Named arguments may appear in any order.
- Reordering happens only when **every** parameter receives an argument.
- If some parameters fall back to defaults, pass the supplied arguments
  positionally, or supply all of them by name.
- A trailing comma after the final argument is not supported.

```ag
func f(a: int, b: int, c: int = 0) { ... }

f(1, 2);              # legal; c uses the default
f(a: 1, b: 2, c: 3);  # legal; every parameter named
f(1, b: 2);           # avoid: partial named form
```

> **Runs.** Fully named and fully positional calls are reliable.

---

## 05.7 Function prototypes and overloads

A declaration without a body is a prototype. It must be followed by a matching
definition somewhere in the program.

```ag
func describe(value: int) -> string;
func describe(value: string) -> string;

func describe(value: int) -> string {
    return "integer";
}

func describe(value: string) -> string {
    return value;
}
```

Overloads are resolved by the argument types. Overloads may also differ by
pointer type:

```ag
func forward(values: int*) -> int* => values;
func forward(table: int**) -> int** => table;
```

> **Runs.** Prototypes and overloads work. Resolution is by arity and argument
> type, not by return type.

### Ambiguity warning

Overload resolution can become ambiguous when an argument has an erased generic
type, because the compiler cannot decide between numeric, character, pointer,
and boolean overloads. If a call is rejected as ambiguous, store the value in a
concretely typed variable first:

```ag
let n: int = compute();
println(n);        # prefer a concrete variable for overloaded functions
```

---

## 05.8 Methods and `self`

Methods live in an `impl` block. The first parameter may be `self` or
`mut self`.

```ag
struct Counter {
    value: int,
}

impl Counter {
    # Associated function: no `self`, called as Counter::make(...)
    func make(start: int) -> Self => Counter { value: start };

    # Read-only method
    func get(self) -> int => self.value;

    # Mutating method
    func add(mut self, n: int) => self.value = self.value + n;
}
```

```ag
var c = Counter::make(5);
c.add(4);
println(c.get());       # 9
```

Rules:

- `Self` in a signature means the type named by the enclosing `impl`.
- A method without `self` is an **associated function** and is called as
  `Type::name(...)`. Writing `static func` is also accepted.
- A method with `self` is called as `value.name(...)`.
- A `mut self` method requires a mutable receiver. Calling it on a `let`
  binding is rejected:

```ag
let frozen = Counter { value: 1 };
frozen.add(1);          # error: cannot assign to immutable variable
```

- `self.field` reads a field; `self.field = value` writes it.
- Member access also accepts `->`, so `self->value` is the same as
  `self.value`.

> **Runs.** Methods, associated functions, `Self`, `self`, and `mut self` are
> implemented.

---

## 05.9 Visibility and modifiers

| Spelling | Meaning today |
| --- | --- |
| `pub func ...` | Marks a declaration public. Import boundaries use it. |
| `priv func ...` | Marks a declaration private. |
| `pub` / `priv` on a field | Controls struct-literal access from other modules. |
| `static func ...` | Treated like an associated function. |
| `static` on a local | Persistent storage (chapter 03). |
| `async func ...` | Parsed; the marker is ignored. |
| `comptime func ...` | Parsed; the marker is ignored. |

```ag
mod tools {
    pub func usable() -> int => 1;
    priv func hidden() -> int => 2;

    pub func call_hidden() -> int => hidden();
}

import tools { usable };
# tools::hidden() from outside is rejected
```

Inside the declaring module, `priv` declarations remain visible.

> **Runs (visibility).** Public/private checks are applied to functions and
> struct fields when crossing modules. `async` and `comptime` do not change
> execution.

---

## 05.10 Attributes, `extern`, and tests

Attributes are accepted and ignored. They may take a parenthesised argument.

```ag
@inline
func clamp(v: int, lo: int, hi: int) -> int => v < lo ? lo : v > hi ? hi : v;

@deprecated("use add instead")
func plus(a: int, b: int) -> int => a + b;
```

`extern "c" { ... }` declares functions whose symbols are provided by a linked C
library, object file, or static archive. External declarations need no AGC body,
and their names are used exactly as written: no module prefix and no AGC type
suffix. The ABI string is informational; it is not validated, and other ABI
strings are accepted syntactically.

```ag
extern "c" {
    func strlen(text: char*) -> int;
    func puts(text: char*) -> int;
}

func main() -> int {
    puts("hello from libc");
    return strlen("abc") - 3;
}
```

```sh
build/agc hello.ag -o /tmp/hello         # libc symbols need no extra flags
/tmp/hello
```

Link a library that is not part of the default C runtime with `-L` and `-l`, or
pass an object/archive path through `--link-arg`:

```sh
# Shared library, with an rpath so the executable can find it at run time.
build/agc app.ag -L /path/to/lib -l mylib \
    --link-arg -Wl,-rpath,/path/to/lib -o /tmp/app

# Static archive or object file.
build/agc app.ag --link-arg /path/to/libmylib.a -o /tmp/app
```

Argument and return values are converted at the C ABI boundary:

| AGC type | C ABI type |
| --- | --- |
| Pointer, reference, array, struct, enum, `string`, `char*` | `ptr` |
| `int` | `i32` (truncated from the AGC i64 slot) |
| `int` return | `i32`, sign-extended to an AGC `int` |
| `uint`, `u32`, `i32` | `i32` |
| `u16`, `i16` | `i16` |
| `u8`, `i8`, `char`, `bool` | `i8` |
| `u64`, `i64` | `i64` |
| `float` | `float` (converted to and from AGC's double-bit slot) |
| Return type `void` | `void` |

Unsigned narrow returns (`u8`, `u16`, `u32`, `uint`, `bool`) are zero-extended
into the AGC slot; signed returns are sign-extended.

Structs by value, C varargs, callbacks/function pointers, and runtime library
loading are not supported. Windows DLLs are not supported; linking is Unix-only.
Pass `null` or `nullptr` for pointer parameters that expect a null handle.

When a C API passes structs by value, wrap the call in a small C shim that takes
scalar or pointer arguments and builds the structs in C. The
[SFML example](../examples/sfml/README.md) does this for `sfVideoMode` and
`sfColor` and links the result with `-L`/`-l`.

> **Runs (Unix link-time).** Extern calls resolve against libc, archives, and
> shared libraries when the executable is linked. The host target is Linux.

`test "name" { ... }` blocks are parsed and compiled out of normal executables.

```ag
test "addition works" {
    assert(add(2, 3) == 5);
}
```

> **Parse-only.** Tests are not run by the compiler and `assert` is not
> resolved.

---

## 05.11 Operator overloading

Two declaration forms are accepted.

### Canonical form

```ag
operator+(left: Type, right: Type) -> ReturnType { ... }
```

### Infix form

```ag
operator (left: Type) + (right: Type) -> ReturnType { ... }
```

Both forms may be used with or without a body (`;` makes a prototype). The infix
form is usually easier to read for binary operators.

```ag
struct Vec2 { x: int, y: int }

operator (a: Vec2) + (b: Vec2) -> Vec2 =>
    Vec2 { x: a.x + b.x, y: a.y + b.y };

operator (a: Vec2) * (scale: int) -> Vec2 =>
    Vec2 { x: a.x * scale, y: a.y * scale };
```

### Index operator

```ag
operator[](value: Vec2, index: int) -> int =>
    index == 0 ? value.x : value.y;

let v = Vec2 { x: 3, y: 4 };
let x = v[0];       # 3
let y = v[1];       # 4
```

### Explicit call form

An operator can also be called by name:

```ag
let sum = operator+(a, b);
```

### Supported symbols

`+ - * / % << >> < <= > >= == != & | ^ && || ! ~ ++ -- [] =` and the compound
assignment operators.

```ag
operator (value: Flag) ! (other: Flag) -> Flag => ...;   # unary
operator (value: Flag) ++ (nothing: int) -> Flag => ...; # postfix-ish form
```

### Resolution

- Calls with a user type on the left resolve to the operator declared for that
  type.
- Built-in integers, characters, floats, and pointers already have built-in
  operator semantics. Declaring operators for those types can override or
  confuse overload resolution, so avoid redefining common operators on `int`,
  `char`, `string`, or pointers unless you need to.

```ag
# Avoid unless deliberate: redefining `+` for int changes existing expressions.
operator (a: int) + (b: int) -> int => a - b;
```

> **Runs.** User-defined binary and index operators work. Not every symbol is
> exercised by the current test suite; prefer the common arithmetic,
> comparison, indexing, and stream forms.

---

## 05.12 Complete runnable example

```ag
# functions.ag
mod ag::io;
import ag::io { println };

struct Counter {
    value: int,
}

impl Counter {
    func make(start: int) -> Self => Counter { value: start };
    func get(self) -> int => self.value;
    func add(mut self, n: int) => self.value = self.value + n;
}

struct Pair {
    a: int,
    b: int,
}

operator (x: Pair) + (y: Pair) -> Pair =>
    Pair { a: x.a + y.a, b: x.b + y.b };

operator[](value: Pair, index: int) -> int =>
    index == 0 ? value.a : value.b;

func greet(name: string = "world") => println(name);

func total(values: int...) -> int {
    var sum = 0;
    for value in values {
        sum += value;
    }
    return sum;
}

func main() -> int {
    greet();
    greet("ag");

    var counter = Counter::make(5);
    counter.add(4);
    println(counter.get());                 # 9

    let p = Pair { a: 1, b: 2 } + Pair { a: 10, b: 20 };
    println(p.a + p.b);                     # 33
    println(p[1]);                          # 22

    println(total(1, 2, 3));                # 6

    let values: int[3] = [4, 5, 6];
    println(total(...values));              # 15
    return 0;
}
```

```sh
build/agc functions.ag -I lib -o /tmp/functions
/tmp/functions
```

Next: [06 — Control flow](06-control-flow.md).

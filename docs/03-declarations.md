# 03 — Declarations

AGC has three value-declaration keywords: `let` (immutable binding), `var`
(mutable binding), and `const` (compile-time-style constant). `static` changes
storage duration. Declarations may appear at module level or inside a block.

---

## 03.1 `let`: immutable binding

```ag
let name: Type = expression;
let name = expression;          # type inferred
let name: Type;                 # declared, must be assigned before use
```

A `let` binding cannot be reassigned or mutated (including its struct fields).
It can still be used to read through a pointer or a mutable reference.

```ag
let answer: int = 42;
let message = "hello";          # inferred string
let flag: bool = true;

answer = 43;                    # error: cannot assign to immutable variable
message[0] = 'H';               # error: cannot write through a let binding
```

### Reading through indirect values

```ag
var x = 1;
let r: &mut int = &mut x;
r = 2;                          # legal: writes through &mut

let p: int* = alloc(8);
p[0] = 5;                       # error: the let binding cannot write through a pointer
```

To write through a raw pointer, use `var`:

```ag
var p: int* = alloc(8);
p[0] = 5;                       # legal
free(p);
```

> **Runs.** `let` immutability is enforced for variables, fields, and pointer
> writes. Mutable references are the exception: the reference value is
> immutable, but its pointee is not.

---

## 03.2 `var`: mutable binding

```ag
var name: Type = expression;
var name = expression;          # inferred
var name: Type;                 # declared, may be assigned later
```

```ag
var count: int = 0;
count = count + 1;
count += 1;
count++;

var text: string = "a";
text = "b";

var buffer: char* = alloc(8);
buffer[0] = 'x';
free(buffer);
```

A `var` declaration without an initializer leaves the slot uninitialised; assign
before reading.

```ag
var value: int;
value = 10;
```

> **Runs.** Both forms are supported. Multiple declarators in one statement
> (`let a = 1, b = 2;`) are **not** supported — write one declaration per
> statement.

---

## 03.3 `const`: constant binding

```ag
const NAME: Type = expression;
```

`const` requires an explicit type and an initializer. The binding is immutable.

```ag
const MAX: int = 100;
const MASK: int = 0xFF_FF;
const GREETING: string = "hello";
const NEWLINE: char = '\n';
```

`const` may be used at module level or inside a function:

```ag
func main() -> int {
    const LOCAL: int = 6;
    return LOCAL - 6;
}
```

> **Important limitation.** Module-level (and static) initializers are placed
> directly in the data section. Only literals — optionally wrapped in unary
> `+`/`-` — are evaluated. A non-literal initializer silently becomes **zero**
> rather than running at startup.

```ag
func compute() -> int => 7;

const GOOD: int = 7;              # 7
const BAD: int = compute();       # currently 0, not 7
var ALSO_BAD = compute();         # currently 0, not 7
```

Use a function or initialise inside `main` when you need a computed value.

---

## 03.4 `static`: storage duration

`static` may prefix `let`, `var`, or `const`. For a local declaration it gives
the value program-lifetime storage; for a module-level declaration it records
static storage.

```ag
static var counter: int = 0;
static let LIMIT: int = 10;
static const BASE: int = 100;
```

A static local keeps its value across calls:

```ag
mod ag::io;
import ag::io { println };

func next() -> int {
    static var value: int = 0;
    value += 1;
    return value;
}

func main() -> int {
    println(next());   # 1
    println(next());   # 2
    return 0;
}
```

For non-literal initializers, a static local uses a once-only guard so the
initializer runs on first execution:

```ag
func make() -> int => 42;

func get() -> int {
    static var value: int = make();   # initialised once
    return value;
}
```

> **Runs.** Static locals persist. Module-level non-literal initializers are
> still zero-initialised (see the `const` limitation above).

---

## 03.5 Type inference

When the type is omitted, it is inferred from the initializer:

```ag
let i = 10;             # int (unsuffixed literals default to int)
let u = 0u;             # uint
let b = false;          # bool
let a8 = 255u8;         # u8
let i16value = -5i16;   # i16
let f = 1.5;            # float
let c = 'x';            # char
let s = "text";         # string
let p = Point { ... };  # struct type
let a = [1, 2, 3];      # pointer type with array_length = 3
let m = map { "a": 1 }; # map pointer type
```

`let`/`var` with no type and no initializer is an error. `const` always requires
a type.

```ag
let x;                  # error: expected initializer
var y;                  # error: expected initializer
```

---

## 03.6 Tuple destructuring

```ag
let (a, b) = make_pair();
var (x, y) = (1, 2);
```

The number of names must match the tuple elements. Each binding gets the
corresponding element type.

```ag
func pair() -> (int, string) => (7, "seven");

func main() -> int {
    let (number, text) = pair();
    return number - 7;      # text is also bound
}
```

> **Runs.** Tuple destructuring works. Tuples themselves cannot be indexed
> directly.

---

## 03.7 Struct destructuring

```ag
let Point { x, y } = point;             # fields bind to same names
let Point { x: px, y: py } = point;     # renamed bindings
```

Fields may be listed in any order. The source expression must be the named
struct type.

```ag
struct Point { x: int, y: int }

func main() -> int {
    let p = Point { x: 3, y: 4 };
    let Point { x, y } = p;
    let Point { x: px, y: py } = p;
    return (x + y) - (px + py);         # 0
}
```

> **Runs.** Struct destructuring is implemented for `let` (and the same parser
> path is used for other declaration forms).

> **Caveat.** Destructured names are ordinary locals. Reusing a name already
> bound in the same scope produces `Redefinition of symbol`.

---

## 03.8 Scope and shadowing

Braces create a scope:

```ag
func main() -> int {
    let x = 1;
    {
        let y = 2;      # separate binding in the inner scope
        # ...
    }
    return x - 1;       # outer x
}
```

Declarations in `if`, `while`, `for`, `with`, and `match` bodies live in their
own scopes. A `match` arm introduces a fresh scope for its bindings.

> **Current quirk.** The symbol table rejects reusing a name from an enclosing
> scope, so the inner `let x = 2;` form above currently fails with
> `Redefinition of symbol`. Reusing the same binding name in sibling `match`
> arms triggers the same error. Use distinct names in patterns when in doubt.

---

## 03.9 Module-level values

At module scope, declarations are qualified with the active module path:

```ag
mod settings {
    pub const WIDTH: int = 80;
    pub var debug: bool = false;

    pub func width() -> int => WIDTH;
}
```

Access them through `settings::WIDTH` or import the names:

```ag
import settings { WIDTH, debug, width };
```

Module-level `var` is global mutable storage. Module-level `let`/`const` is
read-only.

> **Runs.** Global values work. Initializers must be literals (see 03.3).

---

## 03.10 Quick reference

```ag
let x: int = 1;              # immutable, typed
let x = 1;                   # immutable, inferred
let x: int;                  # immutable, assign later

var x: int = 1;              # mutable, typed
var x = 1;                   # mutable, inferred
var x: int;                  # mutable, assign later

const X: int = 1;            # constant; type and initializer required

static var x: int = 0;       # persistent storage
static let X: int = 0;
static const X: int = 0;

let (a, b) = pair;           # tuple destructuring
let Point { x, y } = p;      # struct destructuring
let Point { x: px } = p;     # renamed field binding
```

Next: [04 — Expressions and operators](04-expressions.md).

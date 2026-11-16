# 08 — Structs, enums, and impls

User-defined types are declared with `struct` and `enum`. Methods and
associated functions are added in `impl` blocks.

---

## 08.1 Struct declaration

```ag
struct Name {
    field: Type,
    other: Type,
}
```

Fields may be separated by commas or semicolons; both are accepted, and a
trailing separator before `}` is allowed.

```ag
struct Point {
    x: int,
    y: int,
}

struct Record {
    id: int;
    name: string;
}
```

Fields use their natural size and alignment, capped at 8 bytes. The first field
is at offset `0`; later fields are padded to their alignment. For example:

```ag
struct Small {
    a: u8,          # offset 0, size 1
    b: bool,        # offset 1, size 1
    c: u16,         # offset 2, size 2
    d: int,         # offset 8, size 8
}                   # total size 16, alignment 8
```

A struct made only of `int`/pointer fields still has offsets `0`, `8`, `16`, ...
Struct values are represented by a pointer to their storage.

> **Runs.** Struct declaration, layout, literals, field access, and methods are
> implemented.

---

## 08.2 Struct literals

```ag
let p = Point { x: 1, y: 2 };
```

Field order in the literal does not matter. Every field must be assigned (or
supplied by a spread).

```ag
let a = Point { x: 1, y: 2 };
let b = Point { y: 2, x: 1 };       # same value
```

### Spread

`..value` copies the fields not explicitly listed.

```ag
let p = Point { x: 1, y: 2 };
let moved = Point { x: 10, ..p };   # moved.x = 10, moved.y = 2
```

The spread value must be the same struct type as the literal.

```ag
let bad = Point { x: 1, ..other_struct };   # type error
```

> **Runs.** Out-of-order fields and `..spread` are implemented.

---

## 08.3 Field access and writes

```ag
let p = Point { x: 1, y: 2 };

let x = p.x;        # read
p.x = 10;           # error: p is a let binding
```

Use `var` when you need to write:

```ag
var p = Point { x: 1, y: 2 };
p.x = 10;
p.y += 5;
```

`->` is accepted as an alias for `.`:

```ag
let y = p->y;
```

> **Runs.** Reads, writes, compound assignments, and `->` member access work.
> Writing through a `let` binding is rejected.

---

## 08.4 Destructuring structs

```ag
let Point { x, y } = p;             # bind fields to the same names
let Point { x: px, y: py } = p;     # rename the bindings
```

See chapter 03 for scope and redefinition caveats.

> **Runs.** Struct destructuring is implemented.

---

## 08.5 Enum declaration

```ag
enum Name {
    Unit,
    Positional(Type, Type),
    Named { field: Type, other: Type },
}
```

Variants are separated by commas or semicolons. A variant may have:

- no payload — a unit variant;
- a parenthesised positional payload;
- a braced named payload.

```ag
enum Status {
    Ok,
    Error(string),
    Pending(int),
    Moved { x: int, y: int },
}
```

Layout:

- Offset `0` holds the variant tag (an integer index).
- Payload fields follow at 8-byte offsets: `8`, `16`, ...
- The enum's storage is large enough for the largest payload.
- Enum values are represented by a pointer, like structs.

> **Runs.** Declaration, construction, tag matching, and payload binding work.

---

## 08.6 Enum construction

### Unit variant

```ag
let status = Status::Ok;
```

### Positional payload

```ag
let failed = Status::Error("bad input");
let pending = Status::Pending(3);
```

### Named payload

```ag
let moved = Status::Moved { x: 10, y: 20 };
```

Enum values are used with `match` or `if let` to inspect the variant and extract
payloads (chapter 07).

```ag
match status {
    Status::Ok => println("ok"),
    Status::Error(message) => println(message),
    Status::Pending(code) => println(code),
    Status::Moved { x, y } => println(x + y),
}
```

> **Runs.** All three construction forms are implemented.

---

## 08.7 `impl`: methods and associated functions

```ag
impl Type {
    func associated(args) -> ReturnType { ... }
    func method(self, args) -> ReturnType { ... }
    func mutating(mut self, args) { ... }
}
```

```ag
struct Point {
    x: int,
    y: int,
}

impl Point {
    # Associated function: no receiver, called as Point::make(...)
    func make(x: int, y: int) -> Self => Point { x: x, y: y };

    # Read-only method
    func sum(self) -> int => self.x + self.y;

    # Mutating method
    func shift(mut self, dx: int, dy: int) {
        self.x += dx;
        self.y += dy;
    }
}
```

```ag
var p = Point::make(3, 4);
p.shift(1, 1);
println(p.sum());       # 9
```

Rules:

- `Self` means the enclosing `impl` type.
- A function without `self` is an associated function, called as
  `Type::name(...)`. `static func` is accepted as an explicit spelling.
- `self` receives the value; `mut self` permits writing fields.
- A `mut self` method requires a mutable receiver variable.
- Methods may also be called with `->` on the object: `p->sum()`.

> **Runs.** Methods, associated functions, `Self`, and `mut self` are
> implemented.

---

## 08.8 Generic impls

Generic parameters are written after `impl` and after the type name.

```ag
struct Box<T> {
    value: T,
}

impl<T> Box<T> {
    func get(self) -> T => self.value;
    func replace(mut self, value: T) => self.value = value;
}
```

Generic parameters are erased; `Box<int>` and `Box<string>` share the same
implementation, and `T` is an opaque 8-byte value. Operations that require a
concrete representation may fail inside a generic body.

```ag
# This can fail because `append` needs a concrete element type.
func add(mut self, item: T) {
    self.items.append(item);
}
```

See chapter 10 for the full generic story.

---

## 08.9 Traits and `impl Trait for Type`

Trait declarations and trait impls are parsed, but there is no dynamic dispatch
or trait-bound checking yet.

```ag
trait Shape {
    func area(self) -> float;
    func describe(self) -> string => f"area = {self.area():.2}";
}

struct Circle {
    r: float,
}

impl Shape for Circle {
    func area(self) -> float => 3.0 * self.r * self.r;
}
```

Calling `circle.area()` works because the concrete method is declared in the
trait impl. Calling a method only through a trait object or a generic bound is
not supported.

> **Parse-only (dispatch).** Trait syntax parses; bounds such as `T: Shape` are
> ignored. Concrete methods inside `impl Trait for Type` are still callable on
> the concrete type.

---

## 08.10 Visibility on types and fields

`pub` and `priv` may prefix type declarations and struct fields.

```ag
mod shapes {
    pub struct Point {
        pub x: int,         # constructible/readable from other modules
        priv y: int,        # hidden outside the module
    }

    pub func make(x: int, y: int) -> Point => Point { x: x, y: y };
}
```

From another module, a struct with any private field cannot be constructed with
a literal at all. Use the module's public constructor function instead.

```ag
import shapes { Point, make };

# Error from outside: the type has a private field.
let bad = Point { x: 1, y: 2 };

# Recommended:
let ok = make(1, 2);
```

> **Runs (field visibility).** Private-field checks apply to struct literals
> constructed outside the declaring module.

---

## 08.11 Type aliases

```ag
type Meters = int;
type Pair = (int, int);
type Callback = func(int) -> int;
```

Aliases are resolved during type parsing and do not create a distinct type.

```ag
type Meters = int;

func advance(distance: Meters) -> Meters => distance + 1;
```

Generic aliases parse but are not instantiated:

```ag
type Predicate<T> = func(T) -> bool;    # parse-only
```

> **Runs.** Non-generic aliases work. Generic aliases are syntax only.

---

## 08.12 Method names and keywords

Member access accepts identifiers and contextual spellings, so names such as
`in`, `map`, and `self` can be reached through `.` when they are fields or
methods:

```ag
struct Bag { count: int }

impl Bag {
    func map(self, value: int) -> int => self.count + value;
}

let bag = Bag { count: 1 };
println(bag.map(2));        # 3
```

`with` is a true keyword. Member access can *spell* `object.with(...)`, but you
cannot declare a method with that name; pick another identifier.

---

## 08.13 Complete runnable example

```ag
# types.ag
mod ag::io;
import ag::io { println };

struct Point {
    pub x: int,
    pub y: int,
}

struct Line {
    a: Point;
    b: Point;
}

impl Point {
    func make(x: int, y: int) -> Self => Point { x: x, y: y };
    func sum(self) -> int => self.x + self.y;
    func shift(mut self, dx: int, dy: int) {
        self.x += dx;
        self.y += dy;
    }
}

enum Shape {
    Empty,
    Circle(int),
    Rect { w: int, h: int },
}

func area(shape: Shape) -> int {
    return match shape {
        Shape::Empty => 0,
        Shape::Circle(r) => 3 * r * r,
        Shape::Rect { w, h } => w * h,
    };
}

func main() -> int {
    let p = Point { x: 1, y: 2 };
    let q = Point { y: 20, ..p };       # spread
    println(q.x + q.y);                 # 21

    var r = Point::make(3, 4);
    r.shift(1, 1);
    println(r.sum());                   # 9

    let line = Line { a: p, b: q };
    println(line.a.x + line.b.y);       # 21

    println(area(Shape::Empty));        # 0
    println(area(Shape::Circle(2)));    # 12
    println(area(Shape::Rect { w: 2, h: 3 }));  # 6
    return 0;
}
```

```sh
build/agc types.ag -I lib -o /tmp/types
/tmp/types
```

Next: [09 — Modules and imports](09-modules-imports.md).

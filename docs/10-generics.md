# 10 — Generics

AGC supports generic **syntax** on functions, structs, impls, and aliases. The
implementation is type-erased: there is no monomorphization and no runtime type
information.

> **Status: Runs (erased).** Generic declarations parse and compile, but `T` is
> an opaque 8-byte value compatible with every type.

---

## 10.1 Generic functions

```ag
func identity<T>(value: T) -> T => value;
```

Call it with inferred or explicit type arguments:

```ag
let a: int = identity(5);          # inferred
let b: int = identity<int>(6);     # explicit; arguments are erased
```

Explicit arguments are skipped by the parser and do not change code generation.

```ag
func max<T>(a: T, b: T) -> T => a > b ? a : b;

let m: int = max(3, 8);            # 8
let n: int = max<int>(3, 8);       # same code
```

Multiple parameters and types are allowed:

```ag
func pair<A, B>(first: A, second: B) -> (A, B) => (first, second);
```

---

## 10.2 Generic structs

```ag
struct Box<T> {
    value: T,
}

struct Pair<T, U> {
    first: T,
    second: U,
}
```

Construction infers the generic arguments from the values:

```ag
let b = Box { value: 7 };
let p = Pair { first: 1, second: "one" };
```

You can also write them explicitly; the arguments are erased:

```ag
let b: Box<int> = Box { value: 7 };
```

Generic fields may use arrays and pointers:

```ag
struct Buffer<T> {
    data: T*,
    count: int,
}

struct Slice<T> {
    items: T[],
}
```

---

## 10.3 Generic impls

```ag
impl<T> Box<T> {
    func get(self) -> T => self.value;
    func set(mut self, value: T) => self.value = value;
}
```

```ag
var b = Box { value: 7 };
let v: int = b.get();       # store the erased T in a concrete type
println(v);
b.set(9);
let w: int = b.get();
println(w);                 # 9
```

The same impl serves every instantiation. `Box<int>` and `Box<string>` do not
produce separate code.

---

## 10.4 Const generic parameters

A parameter may be declared as `const Name: Type`:

```ag
struct Ring<T, const N: int> {
    data: T[N],
    head: int,
}
```

The const parameter parses, but it is **not evaluated**. `T[N]` is treated as an
array type without a resolved length. You cannot use `N` in expressions.

```ag
let r = Ring { data: [1, 2, 3], head: 0 };
println(r.head);
```

> **Parse-only (values).** `const N: int` documents intent but does not compute
> or enforce `N`.

---

## 10.5 `where` clauses

Constraints after a signature are parsed and ignored:

```ag
func max<T>(a: T, b: T) -> T where T: Ord => a > b ? a : b;

func largest<T>(items: T[]) -> T? where T: Ord + Copy {
    # ...
}
```

Trait bounds do not select implementations or reject calls. A call is accepted
as long as the erased body compiles for the operations used.

> **Parse-only (bounds).** `where` clauses are accepted syntax with no
> validation or dispatch.

---

## 10.6 Generic type aliases

```ag
type Callback = func(int) -> int;        # ordinary alias
type Predicate<T> = func(T) -> bool;    # parsed, not instantiated
type Table<K, V> = (K, V);
```

> **Parse-only (instantiation).** Generic aliases parse, but `Predicate<int>` is
> not a distinct usable type.

---

## 10.7 Type erasure and compatibility

Because `T` is opaque:

- `T` is compatible with any concrete type.
- A value of type `T` occupies one 8-byte slot.
- Operations inside generic code are checked against the erased
  representation, not the eventual concrete type.
- Overload resolution cannot always decide what an erased `T` is. This is why an
  overloaded call such as `println(b.get())` can be reported as
  **Ambiguous overload resolution**.

Work around ambiguity by binding the result to a concretely typed variable:

```ag
let value: int = b.get();
println(value);             # reliable
```

```ag
let text: string = boxed.get();
println(text);
```

```ag
# Ambiguous when T is erased:
println(b.get());
```

---

## 10.8 What works inside generic code

Simple operations that do not depend on a concrete layout are reliable:

- returning or copying a `T`;
- storing a `T` in a field;
- passing `T` to another function;
- comparing two `T` values with an operator that exists for the eventual type;
- indexing a `T*` or `T[]` with an integer.

Operations that require a concrete element type can fail:

```ag
struct Stack<T> {
    items: T[],
}

impl<T> Stack<T> {
    func add(mut self, item: T) {
        self.items.append(item);    # can fail: T has no concrete layout
    }
}
```

There is no `self.items.append` in the compiler's erased representation, so this
kind of body is rejected with an operand/type error.

> **Practical rule.** Treat generic code as a template for documentation, and
> move layout-dependent operations into non-generic functions.

---

## 10.9 Generic parameters in types

Generic arguments are written with angle brackets and may be nested:

```ag
struct Entry<T> {
    key: string,
    value: T,
}

struct Table<T> {
    entries: Entry<T>[],
    count: int,
}
```

Nested generic arguments are skipped consistently by the type parser:

```ag
let t: Table<int> = Table { entries: [], count: 0 };
```

Generic parameter names are also usable in field types:

```ag
struct Holder<T> {
    single: T,
    many: T[],
    pointer: T*,
    matrix: T[][],
}
```

---

## 10.10 Generics and enum variants

The enum parser does not accept generic parameters today. Declare a non-generic
enum and erase payloads through pointers or a concrete type if needed.

```ag
enum Value {
    Int(int),
    Text(string),
}
```

> **Missing.** Generic enums are not parsed.

---

## 10.11 Complete runnable example

```ag
# generics.ag
mod ag::io;
import ag::io { println };

struct Box<T> {
    value: T,
}

impl<T> Box<T> {
    func get(self) -> T => self.value;
    func set(mut self, value: T) => self.value = value;
}

struct Pair<T, U> {
    first: T,
    second: U,
}

struct Ring<T, const N: int> {
    data: T[N],
    head: int,
}

func identity<T>(value: T) -> T => value;

func max<T>(a: T, b: T) -> T where T: Ord => a > b ? a : b;

func main() -> int {
    var b = Box { value: 7 };
    let first: int = b.get();
    println(first);             # 7

    b.set(9);
    let second: int = b.get();
    println(second);            # 9

    let p = Pair { first: 1, second: "one" };
    let number: int = p.first;
    println(number);            # 1

    let a: int = identity(5);
    println(a);                 # 5

    let e: int = identity<int>(6);
    println(e);                 # 6

    let m: int = max(3, 8);
    println(m);                 # 8

    let ring = Ring { data: [1, 2, 3], head: 0 };
    println(ring.head);         # 0
    return 0;
}
```

```sh
build/agc generics.ag -I lib -o /tmp/generics
/tmp/generics
```

Next: [11 — Memory and pointers](11-memory-pointers.md).

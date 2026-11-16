# 09 — Modules and imports

Modules are namespaces. They may be written inline or loaded from another file.
Imports bring names into the current scope, optionally under an alias.

---

## 09.1 Inline modules

```ag
mod name {
    # declarations
}
```

```ag
mod mathx {
    pub const PI: int = 3;

    pub func square(value: int) -> int => value * value;

    pub struct Point {
        pub x: int,
        pub y: int,
    }
}
```

Declarations inside the module are qualified with its path:

```ag
let n = mathx::square(4);
let p = mathx::Point { x: 1, y: 2 };
```

Nested inline modules are allowed:

```ag
mod outer {
    mod inner {
        pub func value() -> int => 1;
    }

    pub func value() -> int => inner::value() + 1;
}
```

> **Runs.** Inline modules, nesting, qualification, and member lookup work.

---

## 09.2 File modules

`mod path::to::module;` loads a file and treats its declarations as belonging to
the module path.

```ag
mod ag::io;
```

Searches, in order:

1. the directory of the file containing the `mod` statement;
2. each `-I` include directory, in the order given;
3. the current working directory.

The relative filename uses slashes for path components:

```text
mod ag::io;        ->  ag/io.ag
mod utils;         ->  utils.ag
mod app::model;    ->  app/model.ag
```

Load the standard library with `-I lib`:

```sh
build/agc main.ag -I lib -o main
```

A file module does **not** need a matching `mod` line inside the file. A file
loaded as `mod ag::io;` simply contains declarations that become `ag::...`
names.

```ag
# lib/ag/io.ag (excerpt)
pub func println(value: int) { ... }
```

If a module file is named by several importers, it is parsed only once.

> **Runs.** File modules, include paths, nested paths, and load-once behaviour
> are implemented.

---

## 09.3 Import forms

### Import a module and use its short name

```ag
import ag::io;
```

This binds the last path segment (`io`) as an alias:

```ag
io::println(42);
```

The full path also remains usable:

```ag
ag::io::println(42);
```

### Import with an explicit alias

```ag
import ag::io as console;
```

```ag
console::println(42);
```

### Import selected symbols

```ag
import ag::io { println, eprintln };
```

```ag
println(42);
eprintln("problem");
```

A trailing comma inside the braces is allowed.

```ag
import ag::io { println, };
```

### Import everything visible

There is no wildcard import. Use the module alias, or list the names you need.

---

## 09.4 Qualified names

`::` separates module and type path components. It is also used for associated
functions and enum variants.

```ag
mathx::square(3)            # module function
Point::make(1, 2)           # associated function
Status::Ok                  # enum variant
shapes::Point { ... }       # module-qualified struct literal
```

`agc:` (single colon) is reserved for compiler built-ins:

```ag
agc:write("hi", 2);
agc:alloc(16);
```

Do not confuse the two:

```ag
agc:write(...)      # built-in namespace
ag::io::println(...)  # normal module path
```

> **Runs.** Module qualification, associated-function paths, enum paths, and the
> built-in namespace all work.

---

## 09.5 Imports are scoped

An import belongs to the module or block where it appears. Inline module bodies
and nested modules can have their own imports.

```ag
mod tools {
    import ag::io { println };

    pub func hello() {
        println("hello from tools");
    }
}
```

The outer module does not automatically see `println` from that import.

```ag
import tools { hello };
hello();                    # okay

println("outer");           # error unless imported here too
```

Compared with:

```ag
mod ag::io;
import ag::io { println };

import tools { hello };
hello();
println("outer");           # okay
```

> **Runs.** Import scoping follows the module body.

---

## 09.6 Visibility

| Declaration | Visible where |
| --- | --- |
| `pub func`, `pub struct`, `pub enum`, etc. | Everywhere |
| `priv func` or unmarked function | Declaring module and its descendants |
| `pub` field | Readable/constructible outside the module |
| `priv` or unmarked field | Restricted to the declaring module |
| `pub import` / `pub mod` | Accepted; import/module visibility is not enforced yet |

```ag
mod calc {
    pub func add(a: int, b: int) -> int => a + b;
    priv func secret() -> int => 42;

    pub func call_secret() -> int => secret();
}
```

Outside the module:

```ag
import calc { add, call_secret };

add(1, 2);              # okay
call_secret();          # okay; it calls the private function internally
calc::secret();         # error: Private function is not accessible
```

Struct fields follow the same idea:

```ag
# Outside the declaring module, a struct with a private field
# cannot be built with a literal. Use the module's constructor.
```

> **Runs (functions/fields).** Private function and field access is rejected.
> Visibility on imports and modules themselves is currently accepted but not
> enforced.

---

## 09.7 Name resolution order

When the compiler resolves a name, it considers:

1. locals and parameters in the current scope;
2. declarations in the current module;
3. imported symbol aliases;
4. enclosing modules;
5. loaded module paths;
6. compiler built-ins.

If two imports provide the same name, use a qualified path or an explicit
alias.

```ag
import first as one;
import second as two;

one::value();
two::value();
```

> **Note.** Generic call arguments such as `add<int>(...)` are erased; see
> chapter 10.

---

## 09.8 Standard library layout

The repository's `lib/` directory contains:

```text
lib/ag/io.ag        # ag::io
lib/ag/String.ag    # ag::String
```

Typical setup:

```ag
mod ag::io;
mod ag::String;

import ag::io { println };
import ag::String { String, string_from, string_new };
```

```sh
build/agc main.ag -I lib -o main
```

`mod ag::String;` is specially recognised as a file part: it loads
`ag/String.ag`, and declarations inside it live in the `ag` namespace rather
than `ag::String`. This lets `String.ag` define `ag::String` while the file name
still looks like a module path.

> **Runs.** Standard-library modules load with `-I lib`.

---

## 09.9 Declaration order

Within a module, functions may be used before their textual definition. Struct
and enum types must be declared before they are used in a literal or as a
method owner.

```ag
func main() -> int {
    return helper();        # okay: helper is declared later
}

func helper() -> int => 7;
```

```ag
func main() -> int {
    let p = Point { x: 1, y: 2 };   # Point must already exist
    return 0;
}

struct Point { x: int, y: int }
```

Module-level initializers have an additional limitation: only literals are
evaluated (chapter 03).

> **Runs.** Forward function calls work. Keep type declarations above uses.

---

## 09.10 Complete runnable example

`shapes/calc.ag`:

```ag
# shapes/calc.ag
pub func add(a: int, b: int) -> int => a + b;

pub struct Point {
    pub x: int,
    pub y: int,
}

pub func make(x: int, y: int) -> Point => Point { x: x, y: y };

priv func hidden() -> int => 99;

pub func reveal() -> int => hidden();
```

`main.ag`:

```ag
# main.ag
mod calc;
mod ag::io;

import calc;
import calc as c;
import calc { add, Point, make, reveal };
import ag::io { println };

func main() -> int {
    let p: Point = make(3, 4);

    println(add(p.x, p.y));         # 7
    println(calc::add(10, 20));     # 30
    println(c::reveal());           # 99
    return 0;
}
```

```sh
build/agc main.ag -I shapes -I lib -o /tmp/modules
/tmp/modules
```

`-I shapes` makes `shapes/calc.ag` findable for `mod calc;`.

Next: [10 — Generics](10-generics.md).

# 00 — Getting started

## 00.1 Source files

AGC source files use the `.ag` extension and are plain UTF-8 text. A file is a
sequence of top-level declarations and statements:

```ag
# Declarations: functions, structs, enums, impls, modules, imports, globals.
func add(a: int, b: int) -> int => a + b;

# Executable entry point.
func main() -> int {
    return add(2, 3) - 5;
}
```

There is no required header. Imports and module declarations are themselves
statements and may appear before or after other declarations.

---

## 00.2 `main`

The supported shape is a zero-argument `main` returning `int`:

```ag
func main() -> int {
    return 0;
}
```

`main` may also omit the return type, in which case it returns `void`.

The process exit status is the returned integer. `exit(code)` ends the process
immediately from anywhere.

```ag
func main() {
    write("done\n", 5);
    exit(0);
}
```

> **Runs.** A zero-argument `main` is what the examples use. Do not rely on
> `argc`/`argv` parameters yet.

---

## 00.3 Hello world without a library

`write(buffer, count)` and the other `agc:` built-ins are always available
without importing anything. The plain spellings work too.

```ag
func main() -> int {
    write("hello, ag\n", 10);   # buffer, byte count
    return 0;
}
```

```sh
build/agc hello.ag -o /tmp/hello
/tmp/hello
```

`write` does not compute a length; whoever calls it supplies the byte count.
For NUL-terminated literals you can count the visible characters yourself, or
use the I/O library below.

---

## 00.4 Hello world with `ag::io`

The I/O library exposes `println`, `eprintln`, `readln`, and C++-style `cout` /
`cin`. Load the module part with `mod`, then bring names into scope with
`import`:

```ag
mod ag::io;                      # load lib/ag/io.ag

import ag::io { println };       # make println available unqualified

func main() -> int {
    println("hello, ag");        # newline appended automatically
    println(42);
    return 0;
}
```

Compile with the library directory on the include path:

```sh
build/agc hello.ag -I lib -o /tmp/hello
/tmp/hello
```

Everything is also reachable through the qualified module path without the
`import` list:

```ag
mod ag::io;

func main() -> int {
    ag::io::println("hello");
    return 0;
}
```

See [13 — Strings and I/O](13-strings-io.md) for the full API.

---

## 00.5 Program shape and ordering

Top-level declarations are parsed before semantic analysis, so a function may
call another function that is defined later in the same file.

```ag
func later() -> int;          # prototype (optional)

func main() -> int {
    return later() - 7;
}

func later() -> int => 7;      # definition
```

Overloading is allowed when parameter types differ:

```ag
mod ag::io;
import ag::io { println };

func show(value: int) -> void { println(value); return; }
func show(value: char*) -> void { println(value); return; }
```

> **Runs.** Declarations, prototypes, and overloads are supported. See
> [05 — Functions](05-functions.md).

---

## 00.6 The standard library layout

The `lib/` directory is split into module parts. You load only what you need:

| File | Load with | Provides |
| --- | --- | --- |
| `lib/ag/String.ag` | `mod ag::String;` | `ag::String`, string helper functions and operators. |
| `lib/ag/io.ag` | `mod ag::io;` | `println`, `eprintln`, `readln`, `cout`, `cin`, `endl`, `flush`. |
| `lib/ag.ag` | `mod ag;` | Compatibility entry point that loads only `ag::String`. |

```ag
mod ag::String;
mod ag::io;

import ag::String { String, string_from };
import ag::io { println };

func main() -> int {
    var text: String = string_from("hello");
    text.append(", ag");
    text.print();
    println();
    return 0;
}
```

`mod ag::String;` loads `ag/String.ag` into the `ag` namespace. It does not load
other future `ag` parts. See [09 — Modules and imports](09-modules-imports.md).

---

## 00.7 Parse-only mode

`--parse-only` is useful while editing syntax that depends on libraries or
features that are not semantically complete.

```sh
build/agc examples/plan.ag -I lib --parse-only
```

It still requires module files to exist and every statement to be syntactically
valid, but it does not check types or generate code.

---

## 00.8 Recommended learning path

1. Read [01 — Lexical structure](01-lexical.md) and
   [03 — Declarations](03-declarations.md).
2. Skim [02 — Types](02-types.md) and
   [04 — Expressions](04-expressions.md).
3. Work through [05 — Functions](05-functions.md),
   [06 — Control flow](06-control-flow.md), and
   [07 — Patterns and match](07-patterns-match.md).
4. Read [08 — Structs, enums, and impls](08-structs-enums-impls.md) and
   [09 — Modules](09-modules-imports.md).
5. Use [16 — Cheat sheet](16-cheatsheet.md) as a lookup while writing code.

Runnable proof for almost every feature is in `examples/`; each section below
names the closest example file.

# AGC Language Documentation

This folder documents the syntax that the **current** `agc` compiler actually
accepts. It is organised by language area so you can open one short chapter at a
time. Every chapter is written as a reference with commented examples.

The canonical, runnable examples live in [`../examples/`](../examples). When this
documentation and an old design file disagree, the compiler and `examples/`
win.

For a complete Unix FFI build, see the
[SFML window example](../examples/sfml/README.md): it links CSFML through a small
C shim and opens an animated graphical window.

---

## Status labels

Not every parsed construct is finished. Each feature is marked with one of these
labels:

| Label | Meaning |
| --- | --- |
| **Runs** | Parsed, analysed, lowered, and executable with the LLVM backend. |
| **Parse-only** | Accepted by the parser, but semantic analysis or code generation is incomplete. |
| **No-op** | Parsed and lowered, but currently behaves like a simpler construct (for example `spawn` runs synchronously). |
| **Missing** | Not supported by the current compiler. |

If a chapter does not say otherwise, assume the syntax shown **Runs**.

---

## Build and run

Requirements: CMake, a C++ toolchain, and `clang` (used to turn LLVM IR into an
executable).

```sh
# From the repository root.
cmake -S . -B build
cmake --build build -j2

# Build an executable.
build/agc examples/demo.ag -I lib -o /tmp/ag_demo
/tmp/ag_demo
```

The standard library lives in `lib/`, so programs that use `ag::io` or
`ag::String` must pass `-I lib`.

### Compiler options

| Option | Purpose |
| --- | --- |
| `-o <path>` | Build an executable with the LLVM backend. |
| `--llvm-ir [path]` | Emit textual LLVM IR (stdout if no path). |
| `-ir [path]` | Emit AGC's own textual IR. |
| `-json [path]` | Emit a JSON result. |
| `-I <dir>` | Add a module search path. May be repeated. |
| `-L <dir>` | Add a library search path. May be repeated. |
| `-l <name>` | Link a library, for example `-lGL` or `-lm`. May be repeated. |
| `--link-arg <arg>` | Pass one extra argument to the linker (an archive path, `-Wl,...`, and so on). |
| `--parse-only` | Parse only; skip semantic analysis and code generation. |
| `-h`, `--help` | Show the compiler's own help text. |

```sh
# Check that a file parses without requiring every referenced library.
build/agc examples/plan.ag -I lib --parse-only

# Inspect generated LLVM IR.
build/agc examples/demo.ag -I lib --llvm-ir /tmp/demo.ll

# Link a C shared library and record its runtime search path.
build/agc gl.ag -I lib -L /usr/lib -l glfw -l GL -o /tmp/gl

# Link a C static archive or object file.
build/agc app.ag --link-arg ./libmylib.a -o /tmp/app
```

---

## Documentation map

| Chapter | Contents |
| --- | --- |
| [00 — Getting started](00-getting-started.md) | First program, `main`, modules, build/run loop. |
| [01 — Lexical structure](01-lexical.md) | Comments, identifiers, keywords, literals, semicolons. |
| [02 — Types](02-types.md) | Primitive, pointer, reference, array, tuple, optional, function, user types. |
| [03 — Declarations](03-declarations.md) | `let`, `var`, `const`, `static`, inference, destructuring. |
| [04 — Expressions and operators](04-expressions.md) | Calls, member/index access, unary/binary operators, precedence, pipelines, casts. |
| [05 — Functions and operator overloads](05-functions.md) | Signatures, parameters, defaults, variadics, methods, overloads, operators. |
| [06 — Control flow](06-control-flow.md) | `if`, `while`, `until`, `for`, `loop`, `guard`, labels, `with`, `defer`. |
| [07 — Patterns and `match`](07-patterns-match.md) | Every match pattern form and `if let` / `while let`. |
| [08 — Structs, enums, and impls](08-structs-enums-impls.md) | Data declarations, literals, methods, visibility, `Self`. |
| [09 — Modules and imports](09-modules-imports.md) | Inline/file modules, imports, aliases, include paths, visibility. |
| [10 — Generics](10-generics.md) | Generic syntax, type erasure, `where`, explicit type arguments. |
| [11 — Memory, pointers, references](11-memory-pointers.md) | Borrowing, raw pointers, indexing, allocation, built-ins. |
| [12 — Optionals and error-style values](12-optionals-errors.md) | `T?`, `?`, `!`, `orelse`, `??`, `??=`, variant patterns. |
| [13 — Strings and I/O](13-strings-io.md) | Literals, f-strings, `ag::String`, `ag::io`, low-level I/O. |
| [14 — Concurrency and async](14-concurrency-async.md) | `spawn`, `async`, `await`, channel send/receive. |
| [15 — Attributes, compile-time markers, and missing features](15-misc.md) | Attributes, tests, traits, `comptime`, `extern`, missing features, status summary. |
| [16 — Cheat sheet and grammar](16-cheatsheet.md) | Compact reference, precedence table, EBNF-style grammar. |

---

## Three rules that prevent most syntax errors

1. **Comments start with `#`.** `//` is not a comment.
2. **Statements end with `;`.** The main exception is an expression statement
   whose final token is `}` (a block-like expression such as a `match` or
   `loop`). Declarations such as `let` still require their `;`.
3. **Conditions use parentheses.** `if (x)`, `while (x)`, `until (x)`, and
   `guard (x) else { ... }` require them; `for item in values` does not.

```ag
# A one-line comment.

if (x > 0) {
    write("positive\n", 9);
}
```

---

## Minimal complete program

```ag
# hello.ag
func main() -> int {
    write("hello, ag\n", 10);
    return 0;
}
```

```sh
build/agc hello.ag -o /tmp/hello
/tmp/hello
```

The next chapter explains the program structure and adding the standard I/O
module.

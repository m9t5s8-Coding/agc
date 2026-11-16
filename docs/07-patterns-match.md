# 07 — Patterns and `match`

`match` is an expression: it produces a value, and every arm must produce a
compatible type. Patterns are also used by `if let` and `while let`.

---

## 07.1 Basic `match`

```ag
let label = match value {
    0 => "zero",
    1 => "one",
    _ => "many",
};
```

- The scrutinee follows `match`.
- Optional parentheses around the scrutinee are allowed: `match (value) { ... }`.
- Arms are separated by commas.
- Each arm is `pattern => expression`.
- The final arm may omit its trailing comma.
- `_` is the wildcard and should normally be last.

```ag
match value {
    0 => println("zero"),
    _ => println("other"),
}
```

A `match` used as a statement does not need a trailing `;` when the closing `}`
ends the statement. A `match` used as a value follows the surrounding
declaration's semicolon:

```ag
let x = match value { 0 => 1, _ => 2 };    # semicolon for `let`
```

> **Runs.** `match` expressions and statement-form matches are implemented.

---

## 07.2 Wildcard

```ag
_
```

Matches any value and binds nothing.

```ag
let kind = match value {
    0 => "zero",
    _ => "other",
};
```

---

## 07.3 Integer literals

```ag
match code {
    200 => "ok",
    404 => "missing",
    _ => "unknown",
}
```

Integer patterns use decimal literal tokens. Ranges and alternation are written
with the operators below. `true` and `false` are also accepted as literal
patterns:

```ag
match flag {
    true => "on",
    false => "off",
}
```

> Character literal patterns are not part of the current pattern parser. Use an
> integer literal (the character's numeric value) if you need one.

---

## 07.4 Ranges

```ag
10..=99      # inclusive: 10 through 99
10..99       # exclusive: 10 through 98
```

```ag
let size = match value {
    0..=9 => "small",
    10..=99 => "medium",
    _ => "large",
};
```

The bound values must be integer literals.

---

## 07.5 Alternation

List several patterns separated by `|`. The arm matches if any alternative
matches.

```ag
let kind = match value {
    100 | 200 | 300 => "round",
    _ => "other",
};
```

Alternatives may mix literals and ranges:

```ag
0 | 10..=19 | 100 => "special",
```

---

## 07.6 Binding with `@`

`name @ pattern` binds the matched value to `name` while also testing the
pattern.

```ag
let label = match value {
    n @ 10..=99 => n % 2 == 0 ? "even tens" : "odd tens",
    _ => "outside",
};
```

A guard is often used with a binding:

```ag
match value {
    n @ 10..=99 if n % 2 == 0 => "even tens",
    10..=99 => "odd tens",
    _ => "other",
}
```

> The bound name has type `int`.

---

## 07.7 Guards

Add `if condition` after the pattern. The arm is taken only when the pattern
matches **and** the guard is truthy.

```ag
match status {
    Status::Pending(code) if code > 5 => "slow",
    Status::Pending(code) => "fast",
    _ => "other",
}
```

The guard may use bindings introduced by that pattern. Guards are ordinary
expressions tested against zero.

> **Runs.** Guards are implemented.

---

## 07.8 Enum variants

### Unit variants

```ag
enum Status {
    Ok,
    Error(string),
    Pending(int),
}

let kind = match status {
    Status::Ok => 1,
    Status::Error(message) => 2,
    Status::Pending(code) => 3,
};
```

### Positional payloads

Bindings are written as a parenthesised list of names:

```ag
Status::Error(message) => ...
Status::Moved(x, y) => ...
```

The number of bindings must match the number of payload fields.

### Named payloads

Use braces to bind fields by name. Either shorthand or renamed bindings work:

```ag
enum Status {
    Ok,
    Error(string),
    Pending(int),
    Moved { x: int, y: int },
}

let sum = match status {
    Status::Moved { x, y } => x + y,
    _ => 0,
};

# Renamed field bindings:
let renamed = match status {
    Status::Moved { x: px, y: py } => px + py,
    _ => 0,
};
```

The field binding types come from the variant declaration.

> **Runs.** Unit, positional, and named enum patterns are implemented. The
> compiler does not check exhaustiveness; add a `_` arm when in doubt.

---

## 07.9 `Some` / `Ok` style patterns

Patterns without a `::` separator are treated as **opaque variants**. They test
the scrutinee against zero and bind the whole scrutinee value to every name.

```ag
if let Some(value) = maybe {
    # taken when `maybe` is non-zero
}
```

```ag
if let Ok(value) = result {
    # taken when `result` is non-zero
}
```

Important: there is no real `Option`/`Result` type in the compiler, and the
binding receives the scrutinee itself, not a payload extracted from it. These
spellings are useful only with values that are already the desired type.

```ag
# Bind the scrutinee, not an inner payload.
let n: int = 7;
if let Some(v) = n {
    println(v);         # 7
}
```

For real enum payloads, declare an enum and use its qualified variant
(`Maybe::Some(v)`), which does extract the field.

> **Runs (truthiness form).** `Some(v)`/`Ok(v)` parse and test against zero.
> They do not model `Option` or `Result` payloads.

---

## 07.10 Matching assignment and structs

Struct literals and enum constructors are ordinary expressions, so they can be
used as scrutinees.

```ag
let p = Point { x: 3, y: 4 };
let sum = match p {
    _ => p.x + p.y,
};
```

There is no dedicated struct *pattern* outside enum named payloads; destructure
struct values with `let Point { x, y } = p;` (chapter 03) or read fields
directly.

---

## 07.11 Scopes and binding names

Each arm introduces its own scope. Bindings are visible only in that arm's guard
and body.

```ag
match status {
    Status::Error(message) => println(message),
    Status::Pending(message) => println(message),   # same name, new scope
    _ => println("other"),
}
```

> **Caveat.** Reusing a name that is already bound in an *enclosing* scope can
> currently produce `Redefinition of symbol`. If a pattern binding collides with
> a surrounding local, rename the binding.

```ag
let code = 5;

match value {
    Status::Pending(c) => println(c),   # prefer a fresh name
    _ => println(code),
}
```

---

## 07.12 `match` as an expression

All arms must have compatible types. The result type is taken from the first
arm.

```ag
let grade = match score {
    0..=59 => "F",
    60..=69 => "D",
    70..=79 => "C",
    80..=89 => "B",
    _ => "A",
};
```

```ag
let result = match flag {
    0 => 10,
    _ => 20,
};
```

Arms whose body is a call returning `void` make the whole match `void`; use
statement form in that case.

```ag
match value {
    0 => println("zero"),
    _ => println("other"),
}
```

---

## 07.13 `if let` and `while let` reuse

The same patterns work outside `match`:

```ag
if let Status::Error(message) = status {
    println(message);
}

if let n @ 10..=99 = value {
    println(n);
}
```

```ag
while let Status::Pending(code) = next_status() {
    if (code == 0) {
        break;
    }
}
```

Parentheses are not used around the condition in either form.

> **Runs.** Patterns are shared with `match`.

---

## 07.14 Complete runnable example

```ag
# patterns.ag
mod ag::io;
import ag::io { println };

enum Status {
    Ok,
    Error(string),
    Pending(int),
    Moved { x: int, y: int },
}

func classify(status: Status) -> int {
    return match status {
        Status::Ok => 1,
        Status::Error(message) => 2,
        Status::Pending(code) if code > 5 => code,
        Status::Pending(code) => code + 10,
        Status::Moved { x, y } => x + y,
        _ => 0,
    };
}

func main() -> int {
    println(classify(Status::Ok));                  # 1
    println(classify(Status::Error("bad")));        # 2
    println(classify(Status::Pending(9)));          # 9
    println(classify(Status::Pending(1)));          # 11
    println(classify(Status::Moved { x: 3, y: 4 }));# 7

    for value in 0..12 {
        let label = match value {
            0 => "zero",
            1..=3 => "small",
            n @ 4..=9 if n % 2 == 0 => "even mid",
            4..=9 => "odd mid",
            10 | 11 => "round",
            _ => "huge",
        };
        println(label);
    }

    let renamed = match Status::Moved { x: 1, y: 2 } {
        Status::Moved { x: px, y: py } => px + py,
        _ => 0,
    };
    println(renamed);                               # 3
    return 0;
}
```

```sh
build/agc patterns.ag -I lib -o /tmp/patterns
/tmp/patterns
```

Next: [08 — Structs, enums, and impls](08-structs-enums-impls.md).

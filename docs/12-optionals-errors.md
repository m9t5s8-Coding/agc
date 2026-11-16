# 12 — Optionals and errors

AGC has optional **syntax** and zero-testing operators, but no built-in
`Option`/`Result` types. The current convention is: **zero means absent**.

---

## 12.1 Optional type suffix

Append `?` to a type:

```ag
func find(value: int) -> int? => value;

let maybe: int? = find(5);
```

The suffix parses and documents intent. At runtime `T?` has the same
representation as `T`; there is no wrapper, tag, or optional `None` value.
(`null` / `nullptr` is only a pointer literal.)

```ag
func safe(value: int) -> int? {
    if (value > 0) {
        return value;
    }
    return 0;                  # the current "absent" value
}
```

> **Parse-only / no-op.** `T?` does not change storage or checks.

---

## 12.2 Postfix `?` and `!`

Postfix `?` and `!` are accepted and currently do nothing:

```ag
let a = compute()?;        # same value as compute()
let b = compute()!;        # same value
let c = value?;
let d = value!;
```

They may be used on ordinary values:

```ag
let x: int = 5;
println(x?);               # 5
println(x!);               # 5
```

`?` has a parsing rule: it is a postfix operator only when the next token is a
delimiter (`;`, `)`, `,`, `]`, `}`, `:`). Otherwise `?` starts the ternary
operator.

```ag
let a = compute()?;                 # postfix
let b = flag ? 1 : 0;               # ternary
let c = (compute()?) + 1;           # postfix inside parentheses
```

> **No-op.** These operators do not propagate errors or unwrap payloads.

---

## 12.3 `orelse` and `??`

Both operators return the left value when it is non-zero, otherwise the right
value.

```ag
let a = maybe orelse 10;
let b = maybe ?? 10;        # synonym
```

```ag
func identity(value: int) -> int => value;

let present = identity(3) orelse 7;     # 3
let absent  = identity(0) orelse 7;     # 7

let present2 = identity(3) ?? 7;        # 3
let absent2  = identity(0) ?? 7;        # 7
```

The fallback may be a `return` expression:

```ag
func required(value: int?) -> int {
    let n = value orelse return 0;
    return n;
}
```

Or a call:

```ag
func fallback() -> int => 99;

let value = maybe ?? fallback();
```

Precedence: `orelse`/`??` bind looser than the ternary operator and associate to
the right. Parenthesise mixed expressions when in doubt.

```ag
let x = a orelse b ?? c;        # a orelse (b ?? c)
```

> **Runs.** `orelse`, `??`, zero-testing, nested fallbacks, and `orelse return`
> are implemented.

---

## 12.4 `??=`

`??=` assigns the fallback only when the target is zero.

```ag
var maybe: int = 0;

maybe ??= 42;       # 42
maybe ??= 7;        # still 42
```

```ag
var buffer: char* = nullptr;
buffer ??= alloc(16);
```

> **Runs.** `??=` is implemented.

---

## 12.5 What does *not* exist

The following are **not** built into the compiler:

| Spelling | Status |
| --- | --- |
| `Option<T>` | No built-in type. |
| `Result<T, E>` | No built-in type. |
| `Some(x)` / `None` | No constructors. |
| `Ok(x)` / `Err(e)` | No constructors. |
| `null` / `nullptr` | A `void*` pointer literal; not an `Option` value. |
| `?.` optional chaining | Not implemented. |
| `try` blocks | Not implemented. |
| `catch` / exceptions | Not implemented. |

```ag
let value: Option<int> = Some(1);   # undefined type and constructor
let empty = None;                   # undefined constructor
```

Use `0` for an absent integer. Use `null` or `nullptr` for a null pointer:

```ag
let missing: int = 0;               # absent integer
let ptr: int* = nullptr;            # null pointer
```

---

## 12.6 Custom result enums

The reliable way to model errors is to declare your own enum:

```ag
enum Outcome {
    Ok(int),
    Err(string),
}
```

```ag
func divide(a: int, b: int) -> Outcome {
    if (b == 0) {
        return Outcome::Err("divide by zero");
    }
    return Outcome::Ok(a / b);
}
```

Consume it with `match`:

```ag
match divide(10, 2) {
    Outcome::Ok(value) => println(value),
    Outcome::Err(message) => println(message),
}
```

Or with `if let`:

```ag
if let Outcome::Ok(value) = divide(10, 2) {
    println(value);
}
```

> **Runs.** Custom enums with payloads and pattern matching work. They are just
> ordinary enums.

If you name the enum `Result` and its variants `Ok`/`Err`, the spelling looks
familiar, but it is still your own type:

```ag
enum Result {
    Ok(int),
    Err(string),
}

func safe_div(a: int, b: int) -> Result {
    if (b == 0) {
        return Result::Err("zero");
    }
    return Result::Ok(a / b);
}
```

---

## 12.7 `Some` / `Ok` patterns without an enum

Patterns such as `Some(v)` and `Ok(v)` parse as **opaque variants**. They test
the scrutinee against zero and bind the whole scrutinee value.

```ag
let n: int = 7;

if let Some(v) = n {
    println(v);         # 7; v is n, not a payload
}
```

```ag
let code: int = 0;
if let Ok(v) = code {
    # not taken: code is zero
} else {
    println("absent");
}
```

This is useful for zero-testing a value that already has the right type. It does
**not** model an optional or result wrapper.

> **Runs (truthiness form).** There is no payload extraction in these patterns.

---

## 12.8 Error-handling patterns

### Sentinel values

```ag
func find_index(text: char*, needle: char) -> int {
    var i = 0;
    while (text[i] != 0) {
        if (text[i] == needle) {
            return i;
        }
        i += 1;
    }
    return -1;
}

let index = find_index("hello", 'l');
if (index >= 0) {
    println(index);
}
```

### Out-parameters

```ag
func parse(value: int, out: int*) -> bool {
    if (value < 0) {
        return false;
    }
    *out = value;
    return true;
}

var parsed = 0;
if (parse(5, &parsed)) {
    println(parsed);
}
```

### Optional-by-zero

```ag
func maybe(value: int) -> int? => value;

let real = maybe(0) orelse 42;      # 42
let kept = maybe(7) orelse 42;      # 7
```

> **Runs.** All three patterns work with the current representation.

---

## 12.9 Complete runnable example

```ag
# optionals.ag
mod ag::io;
import ag::io { println };

enum Outcome {
    Ok(int),
    Err(string),
}

func may_fail(flag: bool) -> int? {
    if (flag) {
        return 7;
    }
    return 0;
}

func divide(a: int, b: int) -> Outcome {
    if (b == 0) {
        return Outcome::Err("divide by zero");
    }
    return Outcome::Ok(a / b);
}

func required(value: int?) -> int {
    let n = value orelse return -1;
    return n;
}

func main() -> int {
    println(may_fail(1));                   # 7
    println(may_fail(0));                   # 0

    println(may_fail(1) orelse 100);        # 7
    println(may_fail(0) orelse 100);        # 100
    println(may_fail(0) ?? 100);            # 100

    println(may_fail(1)!);                  # 7
    println(may_fail(1)?);                  # 7

    var maybe = 0;
    maybe ??= 42;
    println(maybe);                         # 42
    maybe ??= 7;
    println(maybe);                         # 42

    println(required(5));                   # 5
    println(required(0));                   # -1

    match divide(10, 2) {
        Outcome::Ok(value) => println(value),
        Outcome::Err(message) => println(message),
    }

    if let Outcome::Err(message) = divide(1, 0) {
        println(message);
    }

    return 0;
}
```

```sh
build/agc optionals.ag -I lib -o /tmp/optionals
/tmp/optionals
```

Next: [13 — Strings, formatting, and I/O](13-strings-io.md).

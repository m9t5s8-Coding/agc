# 06 — Control flow

AGC has the usual branching and looping statements, plus a few extras:
`until`, `if let`/`while let`, labelled loops, `loop` expressions,
`guard`, `with`, `defer`, `unsafe`, and `spawn`.

---

## 06.1 `if`, `else if`, `else`

Conditions must be parenthesised. Braces are mandatory for every branch.

```ag
if (condition) {
    # ...
}
```

```ag
if (score >= 90) {
    println("A");
} else if (score >= 80) {
    println("B");
} else {
    println("C");
}
```

Conditions are tested against zero: `0` is false, any other value is true.
`true` and `false` are `bool` literals with values `1` and `0`; a `bool`
occupies one byte in memory.

```ag
if (ready) { ... }          # ready may be an int or bool
if (ptr) { ... }            # non-null pointer is true
```

There is **no** `if` without parentheses, and `else if` is the only chained
form (write nested braces if you need more).

> **Runs.** `if`/`else if`/`else` are implemented.

---

## 06.2 `if let`

`if let` matches a pattern against an expression. The parentheses are **not**
used here.

```ag
if let pattern = expression {
    # pattern bindings are visible here
} else {
    # ...
}
```

```ag
enum Maybe {
    None,
    Some(int),
}

let value = Maybe::Some(7);

if let Maybe::Some(v) = value {
    println(v);
} else {
    println("none");
}
```

Bindings belong to the `if` body scope. The `else` branch is optional. `else if`
works after an `if let` branch too.

> **Runs.** `if let` uses the same pattern engine as `match`. See chapter 07 for
> the pattern forms.

---

## 06.3 `while`

```ag
while (condition) {
    # ...
}
```

```ag
var i = 0;
while (i < 5) {
    println(i);
    i += 1;
}
```

The condition is tested before each iteration. Parentheses are required.

---

## 06.4 `until`

`until (condition)` loops **while the condition is false**; it stops when the
condition becomes true. It is the complement of `while`.

```ag
var i = 0;
until (i >= 5) {
    println(i);
    i += 1;
}
```

Both loops above print `0` through `4`.

> **Runs.** `until` is implemented as an inverted `while`.

---

## 06.5 `while let`

```ag
while let pattern = expression {
    # ...
}
```

```ag
var total = 0;
while let Maybe::Some(v) = Maybe::Some(1) {
    total += v;
    if (total > 3) {
        break;
    }
}
```

The pattern is re-tested every iteration. Parentheses are not used.

> **Runs.** `while let` is implemented.

---

## 06.6 `for` loops

There are three families of `for` loop: integer ranges, iterable values, and
maps.

### Integer ranges

```ag
for i in 0..10 { ... }        # 0, 1, ..., 9      (end exclusive)
for i in 0..=10 { ... }       # 0, 1, ..., 10     (end inclusive)
for i in ..5 { ... }          # 0, 1, ..., 4      (open start)
```

Add a step with `by`:

```ag
for i in 0..100 by 5 { ... }  # 0, 5, 10, ...
```

A negative or expression step is also accepted:

```ag
let step = 2;
for i in 0..10 by step { ... }
```

### Arrays and variadics

```ag
let values: int[4] = [10, 20, 30, 40];

for value in values {
    println(value);
}
```

Add an index binding by listing two names. Both `for i, value in values` and
`for (i, value) in values` are accepted.

```ag
for index, value in values {
    println(f"{index}: {value}");
}
```

The same form works for variadic parameters and array literals:

```ag
func print_all(values: int...) {
    for v in values { println(v); }
}

for v in [1, 2, 3] {
    println(v);
}
```

### Maps

A map literal can be iterated with either one or two names. With two names the
first receives the key and the second the value.

```ag
let scores = map { "ann": 90, "bob": 75 };

for name, score in scores {
    println(name);
    println(score);
}
```

With one name the binding receives the **value** (the map entry). Use the
two-name form when you need the key.

```ag
for score in scores {
    println(score);         # 90, then 75
}
```

> **Runs.** Ranges, `by`, arrays, variadics, array literals, map key/value
> iteration, and index/value iteration are implemented.

### Order and mutation

- Range loops run in the direction implied by the bounds and step.
- Array/map loops use the stored iteration count.
- Mutating the collection while iterating is not checked; avoid it.

---

## 06.7 `break` and `continue`

```ag
func find_first(limit: int) -> int {
    for i in 0..limit {
        if (i * i > 50) {
            break;
        }
        if (i % 2 == 0) {
            continue;
        }
    }
    return 0;
}
```

- `break;` exits the innermost loop (or the targeted labelled loop).
- `continue;` jumps to the next iteration of the innermost loop.
- Both require a trailing `;`.
- Both are only valid inside a loop.

```ag
break;
continue;
```

---

## 06.8 Labelled loops

Prefix a loop with `name:`. Then `break name;` or `continue name;` targets that
loop instead of the innermost one.

```ag
outer: for i in 0..3 {
    for j in 0..3 {
        if (j == 2) {
            continue outer;
        }
        if (i == 2) {
            break outer;
        }
        println(i * 10 + j);
    }
}
```

Labels work on `for`, `while`, `until`, and `loop`.

```ag
search: while (true) {
    break search;
}
```

> **Runs.** Labelled `break`/`continue` are implemented for all loop forms.

---

## 06.9 `loop` and loop expressions

`loop` repeats forever. It is normally used with `break`.

```ag
var count = 0;
loop {
    count += 1;
    if (count >= 5) {
        break;
    }
}
```

A `loop` used as an expression produces the value given to `break`, and `0` if
the loop never breaks with a value.

```ag
var count = 0;
let found = loop {
    count += 1;
    if (count % 7 == 0) {
        break count;
    }
};

println(found);         # 7
```

Rules:

- `break;` leaves the loop with the default value `0`.
- `break expression;` leaves with that value.
- A loop expression has type `int` in the current implementation. Store only
  integer-shaped results.
- Loop **statements** can carry a `label:`. A loop used as an expression cannot
  be labelled (`let x = label: loop { ... }` is rejected); labels are only
  recognised at statement position.

> **Runs.** `loop` statements and loop expressions with `break value` work.
> Labelled loop statements work; labelled loop expressions do not.

---

## 06.10 `guard`

`guard` checks a condition and runs an `else` block when it is false. The
condition must be parenthesised and the `else` block is mandatory.

```ag
guard (condition) else {
    # handle the failure; usually return/break/continue
}
```

```ag
func invert(value: int) -> int {
    guard (value != 0) else {
        return 0;
    }
    return 100 / value;
}
```

The else block must leave the enclosing scope (`return`, `break`, `continue`,
or `exit`). Otherwise execution simply continues after the guard.

> **Runs.** `guard` is implemented as a conditional branch.

---

## 06.11 `with`

`with (let name = expression) { ... }` declares a binding and runs the body in
an inner scope. It is a scoped convenience, not a resource-management construct.

```ag
with (let bonus = base + 5) {
    println(bonus);
    total = bonus;
}
# bonus is no longer visible
```

A `var` binding is also allowed:

```ag
with (var value = 1) {
    value += 1;
    println(value);
}
```

> **Runs.** `with` creates a scoped binding and executes the body. The value is
> not automatically disposed when the block ends.

Member access accepts contextual spellings such as `object.with(...)`, but
`with` is a keyword, so you cannot declare a free function or method *named*
`with`.

---

## 06.12 `defer`

`defer statement;` schedules a statement to run when the enclosing block exits
normally. Multiple defers run in reverse order.

```ag
{
    defer println(1);
    defer println(2);
    println(3);
}
# prints:
# 3
# 2
# 1
```

```ag
func copy(size: int) -> int {
    let buffer: int* = alloc(size);
    defer free(buffer);

    # ... use buffer ...
    return 0;               # cleanup does not run on this early return
}
```

> **Runs, with a limitation.** Defers run on normal block exit, not on an early
> `return`, `break`, or `continue`. Do not rely on `defer` for cleanup paths
> that return early.

---

## 06.13 `unsafe`

`unsafe { ... }` is an annotation block. The body is compiled normally; there is
no restricted-operation checker today.

```ag
unsafe {
    let p: int* = alloc(8);
    *p = 5;
    println(*p);
    free(p);
}
```

> **No-op (marker).** Everything that is legal outside `unsafe` is legal inside
> it; the block is there for intent and future checks.

---

## 06.14 `spawn` and channels

`spawn { ... }` groups statements into a task-shaped block. There is no
scheduler yet, so the block runs synchronously, in place.

```ag
spawn {
    println("runs now");
}
```

Channel syntax is accepted but lowered to pass-throughs:

```ag
let channel = 0;
channel <- 42;          # send marker
let value = <-channel;  # receive marker
```

> **No-op (synchronous).** `spawn` blocks run immediately. `<-` does not block or
> transfer between threads.

---

## 06.15 Semicolons and blocks

- `if`, `while`, `until`, `for`, `loop`, `match`, `with`, `defer`, `unsafe`,
  `spawn`, and bare blocks end with `}` and need no semicolon.
- `break`, `continue`, `return`, `let`, `var`, `const`, and expression
  statements need `;`.
- A `let`/`var`/`const` declaration always needs `;`, even if its initializer
  ends in `}`.

```ag
if (ready) {
    let x = compute();      # semicolon inside
    println(x);
}                           # no semicolon after the if

let value = if_expression;  # semicolon belongs to `let`
```

> There is no `if` expression and no ternary replacement for it; use `match` or
> the `? :` ternary operator when you need a value.

---

## 06.16 Complete runnable example

```ag
# control.ag
mod ag::io;
import ag::io { println };

enum Maybe {
    None,
    Some(int),
}

func classify(n: int) -> string {
    if (n < 0) {
        return "negative";
    } else if (n == 0) {
        return "zero";
    } else {
        return "positive";
    }
}

func main() -> int {
    println(classify(-1));
    println(classify(0));
    println(classify(1));

    if let Maybe::Some(v) = Maybe::Some(7) {
        println(v);
    } else {
        println(0);
    }

    var i = 0;
    while (i < 3) {
        i += 1;
    }
    println(i);                     # 3

    i = 0;
    until (i >= 3) {
        i += 1;
    }
    println(i);                     # 3

    var total = 0;
    for n in 0..4 {
        total += n;
    }
    println(total);                 # 6

    total = 0;
    for n in 0..=4 {
        total += n;
    }
    println(total);                 # 10

    total = 0;
    for n in 0..10 by 2 {
        total += n;
    }
    println(total);                 # 20

    let values: int[4] = [10, 20, 30, 40];
    total = 0;
    for index, value in values {
        total += index + value;
    }
    println(total);                 # 106

    total = 0;
    outer: for a in 0..3 {
        for b in 0..3 {
            if (b == 2) {
                continue outer;
            }
            total += 1;
        }
    }
    println(total);                 # 6

    var count = 0;
    let found = loop {
        count += 1;
        if (count == 4) {
            break count * 10;
        }
    };
    println(found);                 # 40

    guard (count == 4) else {
        return 1;
    }

    with (let bonus = count + 1) {
        println(bonus);             # 5
    }

    {
        defer println(1);
        defer println(2);
        println(3);
    }

    spawn {
        println(99);
    }

    return 0;
}
```

```sh
build/agc control.ag -I lib -o /tmp/control
/tmp/control
```

Next: [07 — Patterns and match](07-patterns-match.md).

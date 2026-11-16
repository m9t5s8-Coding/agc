# 04 — Expressions and operators

Expressions are built from literals, names, calls, member/index access, unary
and binary operators, and a few special forms. This chapter covers the everyday
expression grammar and its precedence.

---

## 04.1 Primary expressions

```ag
42                  # int literal (suffix: 0u, 255u8, -5i8, ...)
3.14                # float literal
'x'                 # char literal
"text"              # string literal
f"value = {x}"      # f-string
true  false         # bool literals
null  nullptr       # null pointer literal (void*)
name                # identifier
(expression)        # grouping
```

An unsuffixed decimal literal is an `int`. A size suffix selects a fixed-width
type: `u` → `uint`, `u8`/`i8`, `u16`/`i16`, `u32`/`i32`, `u64`/`i64`. See
chapter 01 for the full table.

### Tuple literals

A comma makes a parenthesised list a tuple:

```ag
let pair = (1, "one");          # (int, string)
let triple = (1, 2, 3);         # (int, int, int)
let grouped = (1 + 2) * 3;      # parentheses, not a tuple
```

A trailing comma in a tuple literal (`(1,)`) is not supported.

### Array literals

```ag
let empty = [];
let nums = [1, 2, 3];           # trailing comma allowed: [1, 2, 3,]
let nested = [[1, 2], [3, 4]];
```

An empty literal has type `void*`. A non-empty literal is a heap block that
packs elements using their storage width and remembers its element count for
iteration.

```ag
let bytes: u8[3]   = [1u8, 2u8, 250u8];    # three 1-byte elements
let flags: bool[2] = [true, false];        # two 1-byte elements
let words: u16[2]  = [1000u16, 2000u16];   # two 2-byte elements
```

### Map literals

```ag
let scores = map {
    "ann": 90,
    "bob": 75,
};
```

`map` is contextual, so it is still usable as an ordinary identifier. The first
entry determines the key and value types.

### Struct and enum literals

```ag
let p = Point { x: 1, y: 2 };
let moved = Point { x: 5, ..p };            # spread copies the remaining fields
let status = Status::Moved { x: 1, y: 2 };  # named enum payload
```

See chapter 08 for the full rules.

---

## 04.2 Postfix expressions

Postfix operators bind tighter than every binary operator.

| Form | Meaning |
| --- | --- |
| `callee(args)` | Call. |
| `object.member` | Field or method access. |
| `object->member` | Same as `.`; `->` is accepted as an alias. |
| `object[index]` | Index / pointer offset. |
| `object[start..end]` | Range index (currently a no-op slice). |
| `value++`, `value--` | Post-increment / post-decrement. |
| `value?` | Postfix try (no-op). |
| `value!` | Postfix unwrap (no-op). |
| `value as Type` | Cast. |
| `name<T>(...)` | Explicit generic arguments (erased). |
| `module::name` | Qualified path. |
| `agc:name` | Built-in namespace. |

```ag
let first = values[0];
let field = point.x;
let same = point->x;        # same field
let n = compute(1, 2);
let bigger = n as int;
let erased = add<int>(1, 2);
```

### Calls, named arguments, and spread

```ag
func offset(value: int, by: int) -> int => value + by;

offset(5, 2);                  # positional
offset(by: 2, value: 5);       # named, any order
offset(5, by: 2);              # mixed
```

Named arguments are reordered only when **every** parameter receives an
argument. If you leave parameters to their defaults, pass the supplied arguments
positionally or supply all of them by name. A trailing comma after the last
argument is not supported.

Spread passes an existing array/variadic block into the remaining arguments:

```ag
func total(values: int...) -> int { /* ... */ }

let nums: int[3] = [1, 2, 3];
total(...nums);
```

### Member access

`.member` and `->member` both read a struct field or call a method. There is no
automatic dereference distinction.

```ag
let x = point.x;
point.y = 4;
let sum = point.sum();
```

### Indexing

```ag
let value = buffer[0];         # loads one element
buffer[1] = 7;                 # stores one element
let address = buffer + 2;      # pointer arithmetic
```

When `buffer[index]` is passed to a pointer parameter, the address
`buffer + index` is passed instead of the loaded value.

---

## 04.3 Prefix expressions

| Form | Meaning |
| --- | --- |
| `++value`, `--value` | Pre-increment / pre-decrement. |
| `!value`, `not value` | Logical not. |
| `~value` | Bitwise not. |
| `&value` | Address-of / shared reference. |
| `&mut value` | Mutable reference. |
| `*pointer` | Dereference. |
| `+value`, `-value` | Numeric sign. |
| `box value` | Heap marker (currently erased). |
| `comptime value` | Compile-time marker (currently normal evaluation). |
| `await value` | Async wait marker (currently synchronous). |
| `<-channel` | Channel receive marker (currently a pass-through). |

```ag
var i = 1;
++i;                # i is 2
--i;                # i is 1
let negative = -5;
let inverted = ~0;  # -1
let truth = !0;     # 1
let mut_ref = &mut i;
let value = *mut_ref;
let heap = box Point { x: 1, y: 2 };
```

### Increment and decrement

Both prefix and postfix forms work on variables, fields, and elements.

```ag
var i = 1;
let a = i++;        # a = 1, i = 2
let b = ++i;        # b = 3, i = 3
let c = i--;        # c = 3, i = 2
let d = --i;        # d = 1, i = 1
```

---

## 04.4 Binary operators

### Arithmetic

```ag
let a = 7 + 3;      # 10
let b = 7 - 3;      # 4
let c = 7 * 3;      # 21
let d = 7 / 3;      # 2   (integer division truncates)
let e = 7 % 3;      # 1
let f = 7.0 / 2.0;  # 3.5
```

### Comparison

```ag
a == b
a != b
a < b
a <= b
a > b
a >= b
```

Comparisons produce the integer `1` or `0`, including when the operands are
pointers or `bool`, so `!(ptr == other)` and arithmetic on the result work.
Assigning a comparison result back into a `bool` declaration narrows it to
`0`/`1`.

> **String warning.** With plain `string`/`char*` values, `==` compares the
> **addresses**, not the bytes. Use `ag::String` (whose `==` is overloaded) or
> compare characters yourself.

### Logical operators

```ag
a && b          # and
a || b          # or
!a              # not
a and b         # same as &&
a or b          # same as ||
not a           # same as !
```

`&&` and `||` short-circuit in generated code.

### Bitwise operators

```ag
a & b           # bitwise and
a | b           # bitwise or
a ^ b           # bitwise xor
~a              # bitwise not
a << n          # left shift
a >> n          # right shift
```

`<<` and `>>` are also used by `ag::io` as stream operators, resolved by
overload when the left operand is a stream.

---

## 04.5 Assignment operators

```ag
x = value;
x += value;     x -= value;
x *= value;     x /= value;     x %= value;
x &= value;     x |= value;     x ^= value;
x <<= value;    x >>= value;
x ??= value;    # assign only when x is zero/absent
```

Assignment is right-associative:

```ag
a = b = c;      # a = (b = c)
```

The left side must be assignable: a mutable variable, a field of a mutable
value, an element of a writable pointer, or a mutable reference.

```ag
var x = 1;
x += 2;                 # 3

let y = 1;
y += 2;                 # error: immutable

var p = Point { x: 1, y: 2 };
p.x += 5;               # legal

let q = Point { x: 1, y: 2 };
q.x += 5;               # error: q is immutable

var buffer: int* = alloc(16);
buffer[0] = 10;
buffer[0] += 1;
```

---

## 04.6 The ternary operator

```ag
condition ? when_true : when_false
```

```ag
let label = score >= 60 ? "pass" : "fail";
let abs = value < 0 ? -value : value;
let nested = a > b ? a : b > c ? b : c;
```

Both branches must have compatible types. The condition is tested against zero.

---

## 04.7 `orelse` and `??`

These operators return the left value when it is non-zero, otherwise the
fallback.

```ag
let value = maybe orelse 10;
let value2 = maybe ?? 10;          # synonym
```

The fallback may be a `return` expression:

```ag
func required(value: int?) -> int {
    let n = value orelse return 0;
    return n;
}
```

`??=` stores the fallback only when the target is zero:

```ag
var maybe: int = 0;
maybe ??= 42;      # maybe = 42
maybe ??= 7;       # unchanged
```

> **Semantics.** Truthiness is “non-zero”. A null pointer is written `null` or
> `nullptr`; the integer literal `0` is not implicitly converted to a pointer.

---

## 04.8 Casts with `as`

```ag
expression as Type
```

```ag
let wide: int = 300;
let low = wide as char;      # 300 & 0xFF == 44
let back = low as int;       # 44
```

Only `as char` / `as byte` changes the value (low 8 bits). Other casts currently
pass the representation through; they are **not** numeric conversions between
`int` and `float`.

```ag
let f = 1.9;
let i = f as int;            # not 1; the float representation is passed through
```

> **Runs, with limits.** Treat `as` as a bit-level annotation unless the target
> is `char`/`byte`.

---

## 04.9 Membership with `in`

```ag
needle in haystack
```

Supported haystacks:

```ag
2 in 1..=3              # inclusive range
2 in 1..3               # exclusive range
2 in ..3                # open start
"x" in ["x", "y"]       # array literal (address comparison for strings)
```

```ag
let age = 25;
if (age in 18..=65) {
    # adult
}

let valid = code in [10, 20, 30];
```

For a range, `in` compares the needle against the bounds. For an array literal,
it compares against each element. For any other operand it tests the haystack
against zero.

> **Runs.** Parenthesise unusual mixes with other operators; `in` is contextual
> and has intentionally low precedence.

---

## 04.10 Pipelines with `|>`

`value |> fn(args)` passes `value` as the first argument of `fn`.

```ag
func offset(value: int, by: int) -> int => value + by;

let a = 3 |> offset(4);        # offset(3, 4) == 7
let b = 10 |> offset(1) |> offset(2);   # offset(offset(10, 1), 2) == 13
```

If the right side is not a call, it is called with the value as its only
argument:

```ag
let c = 5 |> double;           # double(5)
```

---

## 04.11 Built-in calls (`agc:`)

The built-ins live in a single-colon namespace so they cannot collide with
normal module paths (`::`). The plain names work too.

```ag
write("hello\n", 6);
agc:write("hello\n", 6);       # same function
```

| Built-in | Signature | Purpose |
| --- | --- | --- |
| `alloc(size)` / `malloc(size)` | `(int) -> void*` | Allocate bytes. |
| `free(ptr)` / `dealloc(ptr)` | `(void*) -> void` | Release bytes. |
| `write(ptr, size)` | `(void*, int) -> int` | Write bytes to stdout. |
| `write_err(ptr, size)` | `(void*, int) -> int` | Write bytes to stderr. |
| `read(ptr, size)` | `(void*, int) -> int` | Read bytes from stdin. |
| `exit(code)` | `(int) -> void` | End the process. |
| `memcpy(dst, src, n)` | `(void*, void*, int) -> void*` | Copy bytes. |
| `strcpy(dst, src)` | `(void*, void*) -> void*` | Copy a NUL-terminated string. |

```ag
func main() -> int {
    var buffer: char* = alloc(16);
    strcpy(buffer, "hi");
    write(buffer, 2);
    free(buffer);
    return 0;
}
```

---

## 04.12 Operator overloading syntax

User-defined operators are declared with `operator` (see chapter 05). They are
then invoked with ordinary infix or explicit-call syntax:

```ag
operator (a: Vec2) + (b: Vec2) -> Vec2 => Vec2 { x: a.x + b.x, y: a.y + b.y };

let sum = a + b;            # infix
let same = operator+(a, b); # explicit call
```

Supported operator symbols include `+ - * / %`, shifts, comparisons, bitwise
operators, `&& || ! ~`, `++ --`, `[]`, `=`, and the compound assignments.

---

## 04.13 Precedence

From **highest** to **lowest**:

| Level | Operators / forms | Associativity |
| --- | --- | --- |
| 1 | postfix `()` `[]` `.` `->` `++` `--` `?` `!`, `as Type`, `::`, generic args | left |
| 2 | prefix `++ -- ! ~ & &mut * + -`, `box`, `comptime`, `await`, `<-` | right |
| 3 | `* / %` | left |
| 4 | `+ -` | left |
| 5 | `<< >>` | left |
| 6 | `< <= > >=` | left |
| 7 | `== !=` | left |
| 8 | `in` | left |
| 9 | `&` | left |
| 10 | `^` | left |
| 11 | `\|` | left |
| 12 | `&&`, `and` | left |
| 13 | `\|>` | left |
| 14 | `\|\|`, `or` | left |
| 15 | `? :` | right |
| 16 | `orelse`, `??` | right |
| 17 | `<-` (send), `??=`, `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<=`, `>>=` | right (mixed) |

The three bottom forms (`<-`, `??=`, and assignment) interact in a
position-dependent way. Group mixed expressions explicitly:

```ag
a <- (b = c);
(a <- b) ??= c;
```

When in doubt, add parentheses. The parser prefers clarity over clever
precedence, and a few combinations (especially `in` with comparisons) are easier
to read when grouped.

---

## 04.14 Complete runnable example

```ag
# expressions.ag
mod ag::io;
import ag::io { println };

struct Vec2 {
    x: int,
    y: int,
}

operator (a: Vec2) + (b: Vec2) -> Vec2 =>
    Vec2 { x: a.x + b.x, y: a.y + b.y };

operator (a: Vec2) * (scale: int) -> Vec2 =>
    Vec2 { x: a.x * scale, y: a.y * scale };

func offset(value: int, by: int) -> int => value + by;

func main() -> int {
    let a = 7;
    let b = 3;

    println(a + b);
    println(a - b);
    println(a * b);
    println(a / b);             # integer division
    println(a % b);

    println(a == b);
    println(a != b);
    println(a > b);
    println(a > 0 && b > 0);
    println(a > 100 || b > 0);
    println(!(a == b));

    println(a & b);
    println(a | b);
    println(a ^ b);
    println(~a);
    println(a << 1);
    println(a >> 1);

    var counter = 1;
    counter++;
    counter += 4;
    counter *= 2;
    println(counter);           # (1 + 1 + 4) * 2 = 12

    println(a > b ? a : b);
    println(0 orelse 9);
    println(a ?? 9);

    var maybe = 0;
    maybe ??= 5;
    println(maybe);

    println(3 |> offset(4));
    println(2 in 1..=3);
    println(5 in [1, 2, 3]);

    let low = 300 as char;
    println(low as int);

    let v = Vec2 { x: 1, y: 2 } + Vec2 { x: 10, y: 20 };
    let scaled = v * 2;
    println(scaled.x + scaled.y);

    let boxed = box Vec2 { x: 3, y: 4 };
    println(boxed.x + boxed.y);

    write("built-in write\n", 15);
    return 0;
}
```

```sh
build/agc expressions.ag -I lib -o /tmp/expressions
/tmp/expressions
```

Next: [05 — Functions and operator overloads](05-functions.md).

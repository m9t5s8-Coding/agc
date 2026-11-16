# 01 — Lexical structure

This chapter covers the raw text rules: comments, names, keywords, literals, and
the punctuation the parser recognises.

---

## 01.1 Source files and whitespace

- Source files use the `.ag` extension and plain UTF-8 text.
- Spaces, tabs, carriage returns, and newlines separate tokens but are otherwise
  insignificant. Newlines do **not** terminate statements.
- Identifiers are case-sensitive: `value`, `Value`, and `VALUE` are three names.
- A file may contain a UTF-8 byte-order mark at the start; the lexer skips it.

```ag
func main() -> int {
    let value = 1;      # these two declarations
    let Value = 2;      # are different symbols
    return value + Value - 3;
}
```

---

## 01.2 Comments

A comment starts with `#` and runs to the end of the line.

```ag
# This whole line is a comment.
func main() -> int {
    let answer = 42;    # trailing comments are allowed
    return answer - 42;
}
```

There are **no** block comments and `//` is **not** a comment:

```ag
// This is a syntax error in AGC.
/* Neither is this. */
```

> **Runs.** Only `#` comments exist.

---

## 01.3 Identifiers

An identifier starts with an ASCII letter or `_`, followed by letters, digits, or
`_`.

```ag
let count = 1;
let _internal = 2;
let snake_case_name = 3;
let CamelCase = 4;      # legal, but types conventionally use CamelCase
let value2 = 5;
```

Identifiers may not start with a digit. Unicode letters are not accepted in
identifiers.

Some words are *contextual*: they are ordinary identifiers except in specific
positions. This currently includes:

| Word | Context |
| --- | --- |
| `spawn` | `spawn { ... }` at statement position |
| `in` | `for name in expr` and the membership operator |
| `by` | `for ... in range by step` |
| `where` | generic constraint clause (ignored) |
| `self` | first parameter of a method |
| `map` | `map { key: value }` literal |
| `where`, `Self` | used in types and constraints |

Because they are not reserved, you can still write a variable named `in` or a
field named `with` (member access accepts contextual spellings).

---

## 01.4 Keywords

These are reserved words understood by the lexer:

```text
and or not
func operator struct enum impl type trait
extern comptime
guard orelse
for loop break continue
move box with defer
async await unsafe test
pub priv static mut
import mod as match
true false null nullptr return
char int float double bool void string
uint u8 i8 u16 i16 u32 i32 u64 i64
let var const
if else while until
```

Notes:

- `true` and `false` are lexed as **boolean literals**. Their type is `bool`
  (1 byte in memory), not `int`.
- `null` and `nullptr` are lexed as the same typed pointer literal (type
  `void*`), not as integer literals or identifiers. Chapter 11 covers their use.
- `byte` is a recognised type keyword (alias of `char`) in the type parser, even
  though it is not in the lexer keyword table.
- The fixed-width integer spellings `uint`, `u8`, `i8`, `u16`, `i16`, `u32`,
  `i32`, `u64`, and `i64` are keywords.
- `self`, `Self`, `spawn`, `in`, `by`, `where`, and `map` are contextual, not
  reserved.

---

## 01.5 Integer literals

Integer literals may be decimal, hexadecimal, binary, or octal. Underscores are
allowed anywhere inside the digits for readability.

```ag
let decimal = 1234;
let group   = 1_000_000;    # underscores are ignored
let hex     = 0xFF_FF;      # 65535
let binary  = 0b1010_0110;  # 166
let octal   = 0o755;        # 493
```

A decimal integer literal is an `int` unless it carries a size suffix:

| Suffix | Inferred type | Bytes |
| --- | --- | --- |
| *(none)* | `int` | 8 |
| `u` | `uint` | 4 |
| `u8` / `i8` | `u8` / `i8` | 1 |
| `u16` / `i16` | `u16` / `i16` | 2 |
| `u32` / `i32` | `u32` / `i32` | 4 |
| `u64` / `i64` | `u64` / `i64` | 8 |

Suffixes are case-insensitive, and they may be combined with any base:

```ag
let plain  = 0;        # int
let count  = 0u;       # uint
let byte   = 255u8;    # u8
let signed = -5i8;     # i8 (negation applied to the literal)
let word   = 0xFFu16;  # u16
let wide   = 0x10u64;  # u64
```

`true` and `false` are boolean literals, not integer literals:

```ag
let b_true: bool  = true;
let b_false: bool = false;
```

> **Runs.** All four bases, underscores, and the integer-size suffixes work.
> A literal's suffix determines its inferred type; a declaration may also state
> the type explicitly. Underscores are not allowed between the digits and the
> suffix.

---

## 01.6 Floating-point literals

A float literal contains a decimal point followed by at least one digit, and/or
an exponent introduced by `e`/`E`.

```ag
let simple   = 3.14;
let exponent = 1e9;
let signed   = 2.5e-3;
let grouped  = 1_000.25;
let no_frac  = 2e0;
```

A `.` is only treated as a decimal point when a digit follows it. This keeps
ranges such as `1..5` unambiguous.

```ag
for i in 1..5 {   # a range, not the float 1.5
    let x = 1.5;  # a float
}
```

`float` is stored as a 4-byte IEEE-754 value by the backend. A literal with more
precision than that is rounded at emission time.

---

## 01.7 Character literals

A character literal uses single quotes and holds exactly one byte.

```ag
let newline = '\n';
let tab     = '\t';
let zero    = '\0';
let quote   = '\'';
let slash   = '\\';
let letter  = 'A';      # 65
let digit   = '7';      # 55
```

Supported escapes:

| Escape | Meaning |
| --- | --- |
| `\n` | newline (0x0A) |
| `\r` | carriage return (0x0D) |
| `\t` | tab (0x09) |
| `\0` | NUL byte |
| `\a` | bell |
| `\b` | backspace |
| `\f` | form feed |
| `\v` | vertical tab |
| `\\` | backslash |
| `\'` | single quote |

An unknown escape such as `\q` produces `q` (the backslash is dropped). An
unterminated or empty character literal is a lexer error.

`char` is an 8-bit value. Character literals may be used where an integer is
expected and vice versa; arithmetic uses their numeric value.

---

## 01.8 String literals

A normal string literal uses double quotes. Strings are NUL-terminated byte
buffers; the value has type `string`, which is a `char*` address.

```ag
let greeting = "hello";
let with_nl  = "line one\nline two\n";
let quoted   = "she said \"hi\"";
let embedded = "NUL:\0 done";
let empty    = "";
```

The same escape table as character literals applies, plus `\"` for a double
quote. Unknown escapes keep the escaped character only.

A normal string literal may not span a raw newline. Use a triple-quoted literal
for that.

### Raw strings

Prefix the quote with `r` or `R` to keep backslashes and quotes verbatim. Only a
double quote can end the literal, and there are no escapes.

```ag
let path  = r"C:\Users\name\file.txt";
let regex = r"\d+\.\d+";
let quote = r"a \" still inside";   # a backslash does not escape the quote here
```

### Triple-quoted strings

Three double quotes start a multi-line literal. Newlines are preserved, and the
literal ends at the next `"""`.

```ag
let text = """
line one
line two
""";
```

Indentation inside the literal is part of the string; the compiler does not
strip it.

### Raw + triple + f prefixes

Prefixes may be combined. The recognised spellings are `f`, `F`, `r`, `R`,
`fr`, `rf`, `Fr`, `fR`, `FR`, and `RF` (the prefix must touch the quote).

```ag
let raw_triple = r"""
C:\temp
""";

let formatted = fr"{name}\tend";   # expression is substituted, \t is decoded
```

`fr`/`rf` still decode escapes in the literal segments; only the expression text
is left alone. Use a plain `r"..."` literal when you want backslashes preserved.

> **Runs.** Normal, raw, triple-quoted, and f-strings all produce runtime string
> values.

---

## 01.9 F-strings

`f"..."` (or `F"..."`) interpolates expressions written in `{...}`.

```ag
mod ag::io;
import ag::io { println };

func main() -> int {
    let name = "ag";
    let n = 42;
    println(f"hello, {name}");          # hello, ag
    println(f"{n} + 1 = {n + 1}");      # 42 + 1 = 43
    println(f"{{literal braces}}");     # {literal braces}
    println(f"{n:>6}|{255:#x}");        # "    42|0xff"
    return 0;
}
```

Format spec after a top-level `:`:

```text
[[fill]align][width][.precision][type]
#  optional alternate form (for example #x)
```

Supported pieces:

| Piece | Example | Meaning |
| --- | --- | --- |
| alignment | `:>6`, `:<6` | right or left align; `:^6` is accepted but currently pads on the left |
| fill | `:*>6` | fill character before the alignment character |
| zero padding | `:06` | fill `0`, right aligned, width 6 |
| width | `:8` | minimum field width |
| precision | `:.2` | decimal places for floats (default `6`) |
| type | `:x`, `:X` | hexadecimal output |
| type | `:d`, `:b`, `:o` | accepted, but currently printed in decimal |
| alternate | `:#x` | prefix `0x` for hexadecimal |

`{{` and `}}` produce literal braces. An expression may contain nested braces,
quotes, and `::` module paths.

> **Runs.** Interpolation supports integer, floating-point, character, boolean,
> and string-like values.

---

## 01.10 Operators and punctuation

### Grouping and separators

```text
( )        parentheses / call / tuple
{ }        block / struct or map literal
[ ]        index / array literal / array type
;          statement terminator; also separates struct fields and enum variants
:          type annotation, named argument, map entry, ternary separator
::         module or type path separator
,          argument / field / parameter separator
.          member access
..         exclusive range
...        variadic type suffix or spread argument
@          attribute marker
->         function return type; also member access (alias of `.`)
=>         expression-bodied function / match arm arrow
```

### Arithmetic, comparison, and logic

```text
+  -  *  /  %
++ --
== != < <= > >=
&& || !
&  |  ^  ~
<< >>
```

`<<` and `>>` are shift operators and also overloadable stream operators in
`ag::io`.

### Assignment

```text
=  +=  -=  *=  /=  %=
&=  |=  ^=  <<=  >>=
??=
```

### Other operators

```text
?      ternary separator, postfix try, optional type suffix
??     orelse/coalesce
??=    coalesce-and-assign
|>     pipeline
<-     channel receive (prefix) or send (infix)
&      address-of (prefix) or bitwise-and / reference type
*      dereference (prefix) or multiply
```

> **Note.** `&` is one token with several roles; the parser decides by position.
> `|` is bitwise-or, while `||` is logical-or. `and`, `or`, and `not` are
> keyword aliases for `&&`, `||`, and `!`.

---

## 01.11 Statement termination

Semicolons are required after ordinary statements:

```ag
let x = 1;
x = x + 1;
println(x);
return 0;
```

The main exception is a statement whose final token is `}`. Block-like
expressions and declarations are self-terminating:

```ag
if (x > 0) {
    println("positive");
}                      # no semicolon needed

match x {
    0 => println("zero"),
    _ => println("other"),
}                      # no semicolon needed

loop {                 # statement form
    break;
}
```

A `match` used as a value in a declaration still obeys the declaration's
semicolon:

```ag
let label = match x {
    0 => "zero",
    _ => "other",
};                     # semicolon belongs to `let`
```

Expressions that do **not** end in `}` still need `;`:

```ag
let y = x + 1;
foo();
x++;
```

`let`, `var`, and `const` declarations always need their `;`, even when the
initializer ends with `}`:

```ag
let p = Point { x: 1, y: 2 };   # semicolon required
const ORIGIN: int = 0;          # semicolon required
```

---

## 01.12 Tokens that are easy to get wrong

| You might write | AGC expects |
| --- | --- |
| `// comment` | `# comment` |
| `1_000` in a float exponent | `1e3` (underscores are allowed in the mantissa, not all exponent spellings) |
| `"a" == "a"` to compare text | `ag::String` comparison, or compare bytes |
| `if x > 0` | `if (x > 0)` |
| `for i in 0..n;` | no `;` after a block form |
| `let x: int;` then use | initialise before reading |
| `double` | `float` |

---

## 01.13 Runnable example

```ag
# lexical.ag
mod ag::io;
import ag::io { println };

const MASK: int = 0b1010_0110;

func main() -> int {
    let hex = 0xFF;
    let text = "escaped:\t\"quoted\"";
    let raw = r"no\escape";
    let multi = """
    three
    lines
    """;
    let ch = '\n';
    let label = f"hex={hex:#x} mask={MASK}";

    println(label);
    println(text);
    println(raw);
    println(multi);
    println(ch);
    return 0;
}
```

```sh
build/agc lexical.ag -I lib -o /tmp/lexical
/tmp/lexical
```

Next: [02 — Types](02-types.md).

# 16 — Cheat sheet and grammar

A compact reference for everything the current compiler accepts. Use the
chapters for explanations and caveats; this page is for quick lookup.

---

## 16.1 Minimal program

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

With the standard library:

```ag
mod ag::io;
import ag::io { println };

func main() -> int {
    println("hello");
    return 0;
}
```

```sh
build/agc hello.ag -I lib -o /tmp/hello
```

---

## 16.2 Declarations

```ag
# Values
let name: Type = expression;        # immutable, typed
let name = expression;              # immutable, inferred
let name: Type;                     # declared, assign before use
var name: Type = expression;        # mutable
var name = expression;
var name: Type;
const NAME: Type = expression;      # type + initializer required
static var name: Type = expression; # program-lifetime storage
static let NAME: Type = expression;
static const NAME: Type = expression;

# Destructuring
let (a, b) = pair_expression;
var (x, y) = (1, 2);
let Point { x, y } = point;
let Point { x: px, y: py } = point;

# Functions
func name(params) -> Type { ... }
func name(params) -> Type => expression;
func name(params) { ... }           # void
func name(params) => expression;    # void expression body
func name(params) -> Type;          # prototype

# Types
struct Name { field: Type, other: Type }
enum Name { Unit, Positional(Type), Named { field: Type } }
impl Name { func method(self) -> Type { ... } }
impl Trait for Name { func method(self) -> Type { ... } }
trait Name { func method(self) -> Type; }
type Alias = ExistingType;

# Modules
mod name { ... }                    # inline
mod path::to::module;               # file module
import path;
import path as alias;
import path { name, other };
```

Semicolons are required after every declaration, including `let`/`var`/`const`
whose initializer ends in `}`. Statements ending in `}` need no semicolon.

Attributes and modifiers:

```ag
@inline
@deprecated("reason")
pub func f() -> int => 1;

async func f() -> int => 1;         # marker
comptime func f() -> int => 1;      # marker
static func f() -> int => 1;        # associated function
test "name" { assert(true); }       # compiled out
extern "c" { func f() -> int; }     # linked C symbol; no AGC body needed
```

---

## 16.3 Types

```ag
int                 # 8-byte signed integer (default literal type)
uint                # 4-byte unsigned integer (suffix `u`)
u8  i8              # 1-byte unsigned / signed integer
u16 i16             # 2-byte unsigned / signed integer
u32 i32             # 4-byte unsigned / signed integer
u64 i64             # 8-byte unsigned / signed integer
char                # 1-byte character (zero-extended)
byte                # 1-byte alias for char
float               # 4-byte floating point
double              # 8-byte floating point (recognised; conversions incomplete)
bool                # 1-byte boolean; true/false are bool literals
void                # no value
string              # char* to NUL-terminated bytes

T*                  # pointer to T
T**                 # pointer to pointer
& T                 # shared reference
&mut T              # mutable reference
T[]                 # unsized array / pointer to T
T[N]                # array with known length N
(T, U)              # tuple
func(T, U) -> R     # function type (not first-class)
T?                  # optional marker (no runtime wrapper)
Name<T, U>          # generic type arguments (erased)
Name<T, const N: int>   # const generic parameter (parse-only value)
```

Literal types:

```ag
0          # int
0u         # uint
255u8      # u8
-5i8       # i8
1000u16    # u16
4000i32    # i32
9u64       # u64
18446744073709551615u64   # full u64 range
true       # bool
false      # bool
null       # void* (converts to pointers, not int)
```

Type aliases are transparent:

```ag
type Meters = int;
type Pair = (int, int);
type Callback = func(int) -> int;
```

---

## 16.4 Statements

```ag
# Blocks and expression statements
{ ... }
expression;

# Branching
if (condition) { ... }
if (condition) { ... } else { ... }
if (condition) { ... } else if (condition) { ... } else { ... }
if let pattern = expression { ... } [else { ... }]

# Loops
while (condition) { ... }
until (condition) { ... }                  # loop while false
while let pattern = expression { ... }
for name in range_or_value { ... }
for index, value in values { ... }
for (index, value) in values { ... }
for i in 0..10 { ... }                     # 0..9
for i in 0..=10 { ... }                    # 0..10
for i in ..5 { ... }                       # 0..4
for i in 0..100 by 5 { ... }
loop { ... }
label: for ... { break label; continue label; }
label: while ... { ... }
label: until ... { ... }
label: loop { ... }

# Control transfer
break;
break label;
break value;                               # loop expression value
continue;
continue label;
return;
return expression;

# Guards and scoped helpers
guard (condition) else { return 0; }
with (let name = expression) { ... }
with (var name = expression) { ... }
defer statement;

# Annotation blocks
unsafe { ... }
spawn { ... }

# Matching
match expression { pattern => expression, ... }
```

`if`, `while`, `until`, and `guard` require parentheses. `for`, `if let`, and
`while let` do not.

---

## 16.5 Expressions

```ag
# Primary
42
3.14
'x'
"text"
f"value = {expr}"
true  false
null  nullptr               # null pointer literal (void*)
name
(expression)
(1, "one")                  # tuple literal
[1, 2, 3]                   # array literal
map { "a": 1, "b": 2 }      # map literal
Point { x: 1, y: 2 }        # struct literal
Point { x: 9, ..other }     # struct spread
Status::Moved { x: 1, y: 2 }# enum named payload
Status::Pending(3)          # enum positional payload
Status::Ok                  # enum unit variant
match value { ... }         # match expression
loop { ... break value; }   # loop expression
```

```ag
# Postfix
callee(args)
object.field
object->field
object[index]
object[start..end]          # parses; not a real slice
value++
value--
value?                      # no-op; only before a delimiter
value!                      # no-op
value as Type               # only `as char` masks
name<T>(...)                # generic args erased
module::name
agc:name
```

```ag
# Prefix
++value  --value
!value   not value
~value
&value
&mut value
*pointer
+value   -value
box value                   # no-op
comptime value              # no-op
await value                 # synchronous
<-channel                   # pass-through
move (params) => body       # lambda marker; not callable
```

```ag
# Binary
a * b   a / b   a % b
a + b   a - b
a << b  a >> b
a < b   a <= b  a > b  a >= b
a == b  a != b
a in haystack
a & b   a ^ b   a | b
a && b  a and b
a || b  a or b
a |> f(args)                # pipeline: f(a, args)
a orelse b
a ?? b
channel <- value            # send marker
```

Arithmetic and bitwise results wrap to the operation's width (mixed widths
widen to the larger type; `bool` promotes to `int`; shifts use the left
operand's width). `/`, `%`, `>>`, and ordered comparisons follow the common
type's signedness; `>>` uses the left operand. An unsuffixed literal is `int`,
so `a_u8 + 10` is 64-bit while `a_u8 + 10u8` wraps at 8 bits.

```ag
# Assignment
x = value
x += value   x -= value   x *= value   x /= value   x %= value
x &= value   x |= value   x ^= value
x <<= value  x >>= value
x ??= value                 # assign when x is zero
```

```ag
# Ternary
condition ? when_true : when_false
```

Calls:

```ag
offset(5, 2);
offset(by: 2, value: 5);    # named, all parameters supplied
total(...values);           # spread
```

---

## 16.6 Patterns

```ag
_                                   # wildcard
200                                 # integer literal
10..99                              # exclusive range
10..=99                             # inclusive range
100 | 200 | 300                     # alternation
0 | 10..=19 | 100                   # mixed alternation
n @ 10..=99                         # bind the matched value
n @ 4..=9 if n % 2 == 0             # binding + guard
Status::Ok                          # unit variant
Status::Error(message)              # positional payload binding
Status::Moved { x, y }              # named payload shorthand
Status::Moved { x: px, y: py }      # renamed field bindings
Some(value)                         # opaque zero-test; binds scrutinee
Ok(value)                           # opaque zero-test; binds scrutinee
```

`match` arms use `=>` and are separated by commas. Add `_ =>` because
exhaustiveness is not checked.

---

## 16.7 Precedence

From **highest** to **lowest**:

| Level | Operators / forms | Associativity |
| --- | --- | --- |
| 1 | postfix `()` `[]` `.` `->` `++` `--` `?` `!`, `as Type`, `::`, generic args | left |
| 2 | prefix `++ -- ! not ~ & &mut * + -`, `box`, `comptime`, `await`, `<-` receive | right |
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
| 17 | `<-` send, `??=`, `=`, compound assignments | mixed; parenthesise |

The bottom three forms interact positionally. Group mixed expressions:

```ag
a <- (b = c);
(a <- b) ??= c;
```

Postfix `?` is recognised only when the next token is a delimiter (`;`, `,`,
`)`, `]`, `}`, `.`, `->`, `:`, `?`, `!`, or EOF). Otherwise it starts a ternary.

---

## 16.8 Semicolon rules

| Form | Terminator |
| --- | --- |
| `let`, `var`, `const`, `static` declarations | always `;` |
| Expression statements | `;` unless the last token is `}` |
| `return`, `break`, `continue`, `defer` | `;` |
| `func ... { }`, `struct ... { }`, `enum ... { }`, `impl ... { }` | `}` |
| `if`, `while`, `until`, `for`, `loop`, `match`, `with`, `unsafe`, `spawn`, block | `}` |
| Expression-bodied function (`=> expr`) | `;` after the expression |
| `type Alias = ...` | `;` |
| `mod name { ... }` | `}`; file `mod path;` needs `;` |
| `import ...` | `;` |

---

## 16.9 EBNF-style grammar

Notation: `{ x }` means zero or more, `[ x ]` means optional, `( a | b )` means
choice. Terminals in quotes are literal tokens; `identifier`, `type`, and
`expression` refer to productions above.

```ebnf
program        ::= { top_level } EOF ;

top_level      ::= module_decl
                 | import_decl
                 | declaration
                 | statement ;

module_decl    ::= "mod" path ( ";" | "{" { top_level } "}" ) ;
import_decl    ::= "import" path [ "as" identifier ]
                   [ "{" identifier { "," identifier } [ "," ] "}" ] ";" ;
path           ::= identifier { "::" identifier } ;

declaration    ::= { attribute }
                   [ "pub" | "priv" ]
                   ( value_decl | func_decl | struct_decl | enum_decl
                   | impl_decl | trait_decl | type_decl | test_decl
                   | extern_decl ) ;

attribute      ::= "@" ( identifier | keyword ) [ "(" { any_token } ")" ] ;

value_decl     ::= [ "static" ] ( "let" | "var" | "const" )
                   ( binding | destructure ) [ ":" type ] [ "=" expression ] ";" ;
binding        ::= identifier ;
destructure    ::= "(" identifier { "," identifier } ")"
                 | identifier "{" field_bind { "," field_bind } "}" ;
field_bind     ::= identifier [ ":" identifier ] ;

func_decl      ::= [ "async" | "comptime" | "static" | "pub" | "priv" ]
                   "func" identifier [ generic_params ]
                   "(" [ params ] ")" [ "->" type ] [ "where" bounds ]
                   ( block | "=>" expression ";" | ";" ) ;
params         ::= param { "," param } ;
param          ::= [ "mut" ] ( identifier ":" type | type )
                   [ "=" expression ] ;
variadic_type  ::= type "..." ;
generic_params ::= "<" generic_param { "," generic_param } ">" ;
generic_param  ::= [ "const" ] identifier [ ":" type ] ;
bounds         ::= identifier ":" bound { "+" bound } ;

struct_decl    ::= "struct" identifier [ generic_params ]
                   "{" [ field { ( "," | ";" ) field } [ "," | ";" ] ] "}" ;
field          ::= [ "pub" | "priv" ] identifier ":" type ;

enum_decl      ::= "enum" identifier "{" [ variant { ( "," | ";" ) variant }
                   [ "," | ";" ] ] "}" ;
variant        ::= identifier
                   [ "(" type { "," type } ")"
                   | "{" field { ( "," | ";" ) field } [ "," | ";" ] "}" ] ;

impl_decl      ::= "impl" [ generic_params ] type [ "for" type ]
                   "{" { func_decl } "}" ;
trait_decl     ::= "trait" identifier [ generic_params ] ( block | ";" ) ;
type_decl      ::= "type" identifier [ generic_params ] "=" type ";" ;
test_decl      ::= "test" [ string_literal ] block ;
extern_decl    ::= "extern" [ string_literal ] block ;

statement      ::= block
                 | declaration
                 | if_stmt | while_stmt | until_stmt | for_stmt
                 | loop_stmt | guard_stmt | with_stmt | defer_stmt
                 | unsafe_stmt | spawn_stmt | match_stmt
                 | return_stmt | break_stmt | continue_stmt
                 | expression ";" ;

block          ::= "{" { statement } "}" ;
if_stmt        ::= "if" "(" expression ")" block
                   [ "else" ( if_stmt | block ) ]
                 | "if" "let" pattern "=" expression block
                   [ "else" ( if_stmt | block ) ] ;
while_stmt     ::= "while" "(" expression ")" block
                 | "while" "let" pattern "=" expression block ;
until_stmt     ::= "until" "(" expression ")" block ;
for_stmt       ::= "for" name_list "in" expression [ "by" expression ] block ;
name_list      ::= identifier [ "," identifier ]
                 | "(" identifier "," identifier ")" ;
loop_stmt      ::= "loop" block ;
guard_stmt     ::= "guard" "(" expression ")" "else" block ;
with_stmt      ::= "with" "(" ( "let" | "var" ) identifier "=" expression ")"
                   block ;
defer_stmt     ::= "defer" statement ;
unsafe_stmt    ::= "unsafe" block ;
spawn_stmt     ::= "spawn" block ;
return_stmt    ::= "return" [ expression ] ";" ;
break_stmt     ::= "break" [ identifier ] [ expression ] ";" ;
continue_stmt  ::= "continue" [ identifier ] ";" ;
match_stmt     ::= match_expr [ ";" ] ;

expression     ::= assignment ;
assignment     ::= channel_send [ assign_op assignment ] ;
assign_op      ::= "=" | "+=" | "-=" | "*=" | "/=" | "%="
                 | "&=" | "|=" | "^=" | "<<=" | ">>=" | "??=" ;
channel_send   ::= coalesce { "<-" coalesce } ;
coalesce       ::= ternary { ( "orelse" | "??" ) ternary } ;
ternary        ::= logical_or [ "?" expression ":" expression ] ;
logical_or     ::= pipeline { ( "||" | "or" ) pipeline } ;
pipeline       ::= logical_and { "|>" logical_and } ;
logical_and    ::= bit_or { ( "&&" | "and" ) bit_or } ;
bit_or         ::= bit_xor { "|" bit_xor } ;
bit_xor        ::= bit_and { "^" bit_and } ;
bit_and        ::= membership { "&" membership } ;
membership     ::= equality { "in" haystack } ;
haystack       ::= range | expression ;
equality       ::= relational { ( "==" | "!=" ) relational } ;
relational     ::= shift { ( "<" | "<=" | ">" | ">=" ) shift } ;
shift          ::= additive { ( "<<" | ">>" ) additive } ;
additive       ::= multiplicative { ( "+" | "-" ) multiplicative } ;
multiplicative ::= unary { ( "*" | "/" | "%" ) unary } ;
unary          ::= ( "++" | "--" | "!" | "not" | "~" | "&" | "&mut"
                 | "*" | "+" | "-" | "box" | "comptime" | "await"
                 | "<-" ) unary
                 | postfix ;
postfix        ::= primary { postfix_op } ;
postfix_op     ::= "(" [ arguments ] ")"
                 | "." identifier
                 | "->" identifier
                 | "[" expression "]"
                 | "[" expression ".." [ "=" ] expression "]"
                 | "++" | "--" | "?" | "!"
                 | "as" type
                 | "<" generic_args ">" "(" [ arguments ] ")" ;
arguments      ::= expression { "," expression } [ "," ] ;
generic_args   ::= type { "," type } ;

primary        ::= int_literal | float_literal | char_literal
                 | string_literal | f_string
                 | "true" | "false"
                 | "null" | "nullptr"
                 | identifier
                 | "(" expression ")"
                 | "(" expression "," expression { "," expression } ")"
                 | array_literal
                 | map_literal
                 | struct_literal
                 | match_expr
                 | loop_expr
                 | lambda ;

array_literal  ::= "[" [ expression { "," expression } [ "," ] ] "]" ;
map_literal    ::= "map" "{" [ map_entry { "," map_entry } [ "," ] ] "}" ;
map_entry      ::= expression ":" expression ;
struct_literal ::= path "{" [ field_init { "," field_init } [ "," ] ] "}" ;
field_init     ::= identifier ":" expression | ".." expression ;
match_expr     ::= "match" [ "(" ] expression [ ")" ] "{"
                   match_arm { "," match_arm } [ "," ] "}" ;
match_arm      ::= pattern [ "if" expression ] "=>" expression ;
loop_expr      ::= "loop" "{" { statement } "}" ;
lambda         ::= [ "move" ] "(" [ params ] ")"
                   [ "->" type ] ( "=>" ( block | expression ) | block ) ;

pattern        ::= "_"
                 | int_literal
                 | int_literal ".." [ "=" ] int_literal
                 | pattern "|" pattern
                 | identifier "@" pattern
                 | path [ "(" [ identifier { "," identifier } ] ")"
                        | "{" [ field_bind { "," field_bind } ] "}" ] ;

type           ::= primary_type { type_suffix } ;
primary_type   ::= primitive_type | identifier [ generic_args ]
                 | "(" type { "," type } ")"
                 | "func" "(" [ type { "," type } ] ")" [ "->" type ]
                 | "&" [ "mut" ] type ;
type_suffix    ::= "*" | "[" [ int_literal ] "]" | "?" ;
```

---

## 16.10 Status summary

| Feature | Status |
| --- | --- |
| Declarations, functions, control flow, structs, enums, impls | **Runs** |
| `uint`, sized integers, `bool` (`true`/`false` literals) | **Runs** |
| Modules, imports, visibility | **Runs** (import visibility not enforced) |
| Generics | **Runs (erased)**; generic enums missing |
| Pointers, references, arrays, manual memory | **Runs** (no checks) |
| `orelse`, `??`, `??=`, custom result enums | **Runs** |
| `ag::String`, `ag::io`, f-strings | **Runs** |
| `null` / `nullptr` pointer literal | **Runs** |
| `extern "c"` | **Runs (Unix link-time)**; exact C symbols, no DLLs |
| `where` bounds, generic aliases, traits, tests | **Parse-only** |
| `T?`, postfix `?`/`!`, `comptime`, `box`, attributes | **No-op** |
| `spawn`, `async`, `await`, `<-` | **No-op (synchronous)** |
| `Option`/`Result`, closures, `sizeof`, tuple indexing | **Missing** |

Full details and examples are in chapter 15.

---

## 16.11 Frequent mistakes

```ag
# 1. Wrong comment syntax
// not a comment
# use # instead

# 2. Missing parentheses
if ready { ... }            # error
if (ready) { ... }          # correct

# 3. Missing semicolon on a declaration
let x = 1                   # error
let x = 1;                  # correct

# 4. Assuming string equality compares text
"a" == "a"                  # 0 (address comparison)
# use ag::String or compare bytes

# 5. Assuming `?` unwraps
let x = maybe?;             # no-op; x is maybe

# 6. Assuming spawn starts a thread
spawn { println("now"); }   # runs immediately

# 7. Assuming module-level calls run
const X: int = compute();   # X is 0; only literals are evaluated

# 8. Indexing a tuple
let first = pair[0];        # error; use destructuring

# 9. Expecting a method named `with`
func with(self) { ... }     # error: expected function name

# 10. Forgetting a `_` arm
match value { 0 => ... }    # not exhaustive-checked; add `_ =>`
```

---

## 16.12 Where to go next

- [README](README.md) — index, status labels, build options.
- [00 — Getting started](00-getting-started.md)
- [01 — Lexical structure](01-lexical.md)
- [02 — Types](02-types.md)
- [03 — Declarations](03-declarations.md)
- [04 — Expressions and operators](04-expressions.md)
- [05 — Functions and operator overloads](05-functions.md)
- [06 — Control flow](06-control-flow.md)
- [07 — Patterns and `match`](07-patterns-match.md)
- [08 — Structs, enums, and impls](08-structs-enums-impls.md)
- [09 — Modules and imports](09-modules-imports.md)
- [10 — Generics](10-generics.md)
- [11 — Memory and pointers](11-memory-pointers.md)
- [12 — Optionals and errors](12-optionals-errors.md)
- [13 — Strings, formatting, and I/O](13-strings-io.md)
- [14 — Concurrency and async](14-concurrency-async.md)
- [15 — Attributes, compile-time markers, and missing features](15-misc.md)

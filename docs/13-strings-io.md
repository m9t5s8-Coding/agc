# 13 — Strings, formatting, and I/O

`string` is a raw `char*` to a NUL-terminated byte buffer. `ag::String` is a
standard-library owned, resizable string. I/O is provided by `ag::io` and a
small set of compiler built-ins.

---

## 13.1 Plain strings

```ag
let greeting: string = "hello";
let bytes: char* = "hello";         # same representation
```

- A string literal is stored statically and must not be freed.
- Indexing yields a byte: `greeting[0]`.
- The value is NUL-terminated, so manual loops stop at `0`.
- `string` and `char*` are interchangeable in most positions.
- `==` on plain strings compares addresses, not contents.

```ag
func length(text: char*) -> int {
    var n = 0;
    while (text[n] != 0) {
        n += 1;
    }
    return n;
}

let n = length("hello");        # 5
let h: char = "hello"[0];       # 'h'
```

Indexing a plain `char*` yields a byte. The compiler may treat the loaded value
as an integer in an overloaded call, so store it in a `char` variable when you
want character behavior:

```ag
let raw = "hello";
println(raw[1]);                # 101 (byte value)
let ch: char = raw[1];
println(ch);                    # e
```

To compare text, use `ag::String`, whose `==` is overloaded:

```ag
mod ag::String;
import ag::String { String, string_from };

let a: String = string_from("hello");
let b: String = string_from("hello");
println(a == b ? 1 : 0);        # 1
```

> **Warning.** `"a" == "a"` on plain `char*` is an address comparison and is not
> a reliable content check.

---

## 13.2 Low-level built-ins

The compiler provides these functions both plain and under `agc:`:

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
write("hello\n", 6);
agc:write("hello\n", 6);        # same function
```

`write` does not add a newline and does not stop at `0`; it writes exactly the
requested number of bytes.

```ag
let text = "hello\n";
write(text, 6);                 # includes the newline
```

> **Runs.** All built-ins above are implemented. `alloc`/`free` are also
> reachable by their plain names.

---

## 13.3 `ag::io` output

```ag
mod ag::io;
import ag::io { cout, endl, println, eprintln, flush };
```

### `println`

Overloads exist for `int`, `char*`, `char`, and `bool`:

```ag
println();              # blank line
println(42);            # 42
println("text");        # text
println('x');           # x
println("done");
```

`true` and `false` are `bool` literals, so they select the `bool` overload:

```ag
println(true);          # true
println(false);         # false
let flag: bool = true;
println(flag);          # true
```

### `eprintln`

Writes to stderr. Overloads mirror `println` for `int`, `char*`, `char`, and
`bool`.

```ag
eprintln(1);
eprintln("problem");
```

### Stream syntax

`cout` is a sentinel stream value; `<<` is overloaded for it.

```ag
cout << "hello " << 42 << endl;
```

`endl` is a newline character. `flush` is a no-op marker.

```ag
cout << "hello" << endl;
cout << flush;          # no effect
```

The `bool` overload applies to literals too:

```ag
cout << true << endl;   # prints true
let flag: bool = false;
cout << flag << endl;   # prints false
```

> **Runs.** `println`, `eprintln`, `cout <<`, `endl`, and `flush` work.

---

## 13.4 `ag::io` input

```ag
import ag::io { cin, cout, endl, readln };
```

### `readln`

Reads one line (without the trailing newline) into a persistent buffer. The
returned pointer is valid until the next read.

```ag
let line = readln();
println(line);

let name = readln("name: ");    # prints the prompt first
println(name);
```

### `cin >>`

`>>` is overloaded for reading an `int` or a line:

```ag
var number: int = 0;
cin >> number;

var text: char* = "";
cin >> text;
```

```ag
cout << "value: ";
var value: int = 0;
cin >> value;
cout << value << endl;
```

Integer parsing skips leading spaces and tabs and accepts an optional sign.
Non-numeric input parses as `0`.

```sh
printf '42\n' | ./program
```

> **Runs.** `readln` and `cin >>` work with piped or interactive input.

---

## 13.5 F-strings

```ag
let name = "ag";
let n = 42;

println(f"hello, {name}");
println(f"{n} + 1 = {n + 1}");
println(f"{{literal braces}}");
```

An expression may contain arithmetic, calls, field access, and `::` paths.

```ag
println(f"value = {compute(1, 2)}");
println(f"name = {point.x}");
println(f"io = {ag::io::constant}");
```

### Format specifiers

After a top-level `:` inside the braces:

```text
{expression:[[fill]align][width][.precision][type]}
```

```ag
println(f"[{n:6}]");        # right aligned, width 6
println(f"[{n:<6}]");       # left aligned
println(f"[{n:>6}]");       # right aligned
println(f"[{n:06}]");       # zero padded
println(f"[{n:*>6}]");      # fill with '*'
println(f"[{255:#x}]");     # 0xff
println(f"[{255:X}]");      # FF
println(f"[{3.14159:.2}]"); # 3.14
```

Supported today:

| Piece | Notes |
| --- | --- |
| `<` / `>` alignment | Implemented. |
| `^` alignment | Accepted, but currently behaves like right alignment. |
| fill character | Implemented (`*`, `0`, ...). |
| width | Implemented. |
| `.precision` | Floats only; default is 6 decimals. |
| `x` / `X` | Hexadecimal. |
| `#` | Adds `0x` to hexadecimal output. |
| `d` / `b` / `o` | Accepted, but currently printed in decimal. |

```ag
println(f"{n:>6}|{255:#x}|{3.14159:.2}");
# "    42|0xff|3.14"
```

> **Runs.** Width, fill, `<`/`>`, float precision, and hexadecimal are reliable.

---

## 13.6 `ag::String`

`ag::String` is an owned, resizable byte buffer declared in `lib/ag/String.ag`.

```ag
mod ag::String;
import ag::String { String, string_from, string_new };
```

### Creating strings

```ag
var a: String = string_new();                  # empty, initial capacity
var b: String = string_with_capacity(32);      # preallocated
var c: String = string_from("hello");          # copy of a char*
var d: String = string_from_char('x');         # one character
var e: String = string_from_int(123);          # decimal text
```

`string_length(text)` returns the length of a plain `char*`, and
`string_to_int(text)` parses decimal text:

```ag
let n = string_length("hello");                # 5
let value = string_to_int("42");               # 42
```

### Reading and indexing

```ag
let s: String = string_from("hello");

println(s.size());         # 5
println(s.length());       # 5
println(s.capacity());     # capacity in bytes
println(s.empty() ? 1 : 0);
println(s.data());         # char* view
println(s.c_str());        # char* view
println(s.at(1));          # 'e'
println(s.front());        # 'h'
println(s.back());         # 'o'
println(s[1]);             # 'e' through operator[]
```

> Indexing and `at` read a character; use `set(index, ch)` to write one.

### Growing and shrinking

These methods take `mut self`, so the variable must be `var`:

```ag
var s: String = string_from("hello");

s.push('!');                  # append one char
s.push_back('?');             # same
s.append(" world");           # append char*
s.append_string(other);       # append another String
s += " more";                 # operator+=
s += '!';
s << " stream";               # operator<< appends
```

```ag
var s: String = string_from("hello");
s.set(0, 'H');                # write one character
s.clear();                    # size 0
s.reserve(64);                # ensure capacity
s.resize(10, '.');            # grow/shrink, filling with '.'
s.pop_back();                 # remove last character
```

### Insert, erase, replace

```ag
var s: String = string_from("Hello World");

s.insert(5, ",");             # "Hello, World"
s.erase(0, 6);                # removes 6 bytes from index 0
s.replace(0, 5, "Earth");     # replace 5 bytes at index 0
```

### Searching and comparing

```ag
let s: String = string_from("Hello World");

println(s.find("World"));         # 6
println(s.find_char('o'));        # 4
println(s.find_from("o", 5));     # 7
println(s.rfind("o"));            # 7
println(s.rfind_char('o'));       # 7
println(s.contains("World") ? 1 : 0);
println(s.starts_with("Hello") ? 1 : 0);
println(s.ends_with("World") ? 1 : 0);
println(s.compare(other));        # negative / zero / positive
println(s.compare_cstr("Hello World"));  # 0
println(s.equals(other) ? 1 : 0);
println(s.equals_cstr("Hello World") ? 1 : 0);
```

### Transform, split, clone

```ag
var s: String = string_from("Hello");
s.to_upper();                  # HELLO
s.to_lower();                  # hello

let part: String = s.substr(0, 3);   # "Hel"
let copy: String = s.clone();        # independent copy
println(s.to_int());                 # parse decimal text
```

There is no `split` method; use `find`/`substr` loops.

### Operators

`ag::String` overloads:

```ag
let a: String = string_from("Hello");
let b: String = string_from(" World");

let c = a + b;            # String + String
let d = a + "!";          # String + char*
let e = a + '!';          # String + char
let f = ">" + a;          # char* + String
```

Comparison operators work by content:

```ag
a == b
a != b
a < b
a > b
a <= b
a >= b
```

Streams and assignment:

```ag
var s: String = string_from("a");
s += "b";
s += 'c';
s << "d";
```

### Printing and freeing

```ag
var s: String = string_from("hello");
s.print();                 # writes to stdout (no newline)
s.deallocate();            # free the buffer
s.drop();                  # alias for deallocate
```

`print` does not add a newline. `deallocate`/`drop` take `mut self`, so the
variable must be `var`:

```ag
let frozen: String = string_from("x");
frozen.deallocate();       # error: cannot assign to immutable variable
```

> **Runs.** The factory functions, methods, and operators documented above are
> implemented in `lib/ag/String.ag`.

---

## 13.7 Mixing plain strings and `ag::String`

Plain strings are cheap views; `String` owns memory.

```ag
let raw = "hello";
var owned: String = string_from(raw);

owned.append(" world");
println(owned.data());         # pass the char* view to I/O

println(raw);                  # raw is unaffected
```

Do not free the result of `.data()`/`.c_str()`; free the `String` with
`deallocate()`.

```ag
var s: String = string_from("hello");
let view = s.data();
# free(view);                  # wrong: view is owned by s
s.deallocate();                # correct
```

---

## 13.8 Complete runnable example

```ag
# strings.ag
mod ag::io;
mod ag::String;

import ag::io { cout, endl, println, eprintln };
import ag::String { String, string_from, string_from_int };

func main() -> int {
    # Plain string
    let raw = "hello";
    println(raw);
    println(raw[1]);                    # 101 (byte value)
    let ch: char = raw[1];
    println(ch);                        # e
    write("bytes\n", 6);

    # Formatting
    let n = 42;
    println(f"hello, {raw}");           # hello, hello
    println(f"{n} + 1 = {n + 1}");      # 42 + 1 = 43
    println(f"[{n:>6}]");               # [    42]
    println(f"[{255:#x}]");             # [0xff]
    println(f"{3.14159:.2}");           # 3.14

    # Streams
    cout << "stream " << n << endl;
    eprintln("to stderr");

    # Owned string
    var text: String = string_from("Hello");
    text.append(" World");
    text.push('!');
    println(text.data());               # Hello World!
    println(text.size());               # 12

    println(text.find("World"));        # 6
    println(text.contains("Hello") ? 1 : 0);

    var sub: String = text.substr(0, 5);
    println(sub.data());                # Hello
    sub.deallocate();

    var number: String = string_from_int(123);
    println(number.to_int());           # 123
    number.deallocate();

    text.deallocate();
    return 0;
}
```

```sh
build/agc strings.ag -I lib -o /tmp/strings
/tmp/strings
```

Next: [14 — Concurrency and async](14-concurrency-async.md).

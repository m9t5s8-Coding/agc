# 11 — Memory and pointers

AGC is a manually managed language. `alloc`/`malloc` reserve bytes and
`free`/`dealloc` release them. Pointers, references, and arrays are the tools for
working with that memory.

---

## 11.1 Allocating and freeing

```ag
var p: int* = alloc(8);      # 8 bytes = one int slot
*p = 42;
free(p);
```

```ag
var buffer: char* = alloc(16);
buffer[0] = 'h';
buffer[1] = 'i';
buffer[2] = 0;
free(buffer);
```

| Built-in | Signature | Notes |
| --- | --- | --- |
| `alloc(size)` | `(int) -> void*` | Allocate `size` bytes. |
| `malloc(size)` | `(int) -> void*` | Same as `alloc`. |
| `free(ptr)` | `(void*) -> void` | Release a block. |
| `dealloc(ptr)` | `(void*) -> void` | Same as `free`. |

`void*` converts implicitly to any object pointer, so no cast is needed:

```ag
let numbers: int* = alloc(24);
```

There is no bounds checking, no allocation-failure check, and no automatic
cleanup. Allocate enough bytes and free exactly once.

```ag
let count = 16;
let bytes: char* = alloc(count);
# ... use bytes[0] .. bytes[count - 1] ...
free(bytes);
```

> **Runs.** Allocation, deallocation, indexing, and pointer conversion work.

---

## 11.2 Pointer types

```ag
T*          # pointer to T
T**         # pointer to pointer to T
void*       # untyped block address
char*       # byte pointer; interchangeable with `string` in most places
```

```ag
var a: int = 5;
var p: int* = &a;        # pointer to a local
println(*p);             # 5

var pp: int** = alloc(8);
*pp = p;
println((*pp)[0]);       # 5
```

A pointer to a struct or enum is the same as the aggregate's representation:

```ag
struct Point { x: int, y: int }

let point = Point { x: 1, y: 2 };
let ptr: Point* = &point;
println(ptr.x);          # 1; struct values already act like pointers
```

> **Runs.** Single and multiple indirection work. Struct values are pointers to
> their storage, so `&struct` yields that same address.

---

## 11.3 Address-of and dereference

```ag
&value          # address of a value
&mut value      # mutable reference
*pointer        # load/store through a pointer
```

```ag
var x = 10;
let p = &x;

println(*p);     # 10
*p = 20;         # writes x
println(x);      # 20
```

Dereference is also how you store through an out-parameter:

```ag
func set_out(out: int*) {
    *out = 7;
}

var value = 0;
set_out(&value);        # value is now 7
```

> **Runs.** Address-of, dereference reads, and dereference writes are
> implemented.

---

## 11.4 Pointer arithmetic and indexing

```ag
p[i]        # element i
*(p + i)    # same element
p + n       # pointer offset by n elements
```

```ag
var numbers: int* = alloc(24);      # three ints
numbers[0] = 10;
numbers[1] = 20;
numbers[2] = 30;

println(numbers[0] + numbers[1] + numbers[2]);  # 60
println(*(numbers + 1));                        # 20

let tail = numbers + 2;
println(*tail);                                 # 30
```

The offset is measured in **elements**, not bytes. The compiler scales by the
pointee size where the type is known.

> **Runs.** `p[i]`, `*(p + i)`, and `p + n` are implemented.

**Not implemented:**

- pointer subtraction (`p - q`);
- pointer comparison beyond ordinary integer comparison of addresses;
- `sizeof`;
- bounds checks;
- automatic pointer invalidation checks.

---

## 11.5 Passing pointers and indexing address semantics

When `buffer[index]` is passed directly to a pointer parameter, the compiler
passes the **address** `buffer + index` rather than the loaded value.

```ag
func fill(mut out: char*) {
    out[0] = 'o';
    out[1] = 'k';
    out[2] = 0;
}

var buffer: char* = alloc(8);
fill(buffer[0]);        # passes &buffer[0]
write(buffer, 2);       # "ok"
free(buffer);
```

Index writes through a pointer need the parameter binding to be mutable, hence
`mut out: char*`. A dereference write (`*out = value`) is accepted without
`mut`.

To pass a loaded value instead, store it first or dereference explicitly:

```ag
let first = buffer[0];
fill(&first);           # a copy
```

> **Runs (special rule).** Remember that `array[index]` in a pointer-argument
> position is an address.

---

## 11.6 References

A reference is a transparent alias with two mutability forms:

```ag
&T          # shared reference; cannot write through it
&mut T      # mutable reference; can write through it
```

```ag
func add_one(value: &mut int) {
    value += 1;
}

func read(value: &int) -> int => value;

func main() -> int {
    var x = 1;
    add_one(&mut x);
    println(read(&x));      # 2
    return 0;
}
```

Using a reference:

- `ref` reads the pointee;
- `ref = value` writes through the reference;
- `*ref` is also accepted for reads and writes.

```ag
var x = 1;
let r: &mut int = &mut x;

r = 2;          # same as *r = 2
println(x);     # 2
println(*r);    # 2
```

Rules:

- A `let` binding may hold `&mut T`; the binding is immutable but the pointee is
  writable through it.
- A shared `&T` rejects writes.
- There is no lifetime or aliasing checker. Do not keep references to storage
  that has been freed or has gone out of scope.

> **Runs.** References are implemented for parameters, locals, fields, and
> elements. Use them carefully.

---

## 11.7 `let` versus `var` writes

Writing through a pointer requires the pointer's **binding** to be mutable:

```ag
let p: int* = alloc(8);
p[0] = 5;               # error: cannot assign to immutable variable
free(p);
```

```ag
var p: int* = alloc(8);
p[0] = 5;               # legal
free(p);
```

Writing through `&mut` does not require the binding itself to be `var`:

```ag
var x = 0;
let r: &mut int = &mut x;
r = 5;                  # legal
```

> **Runs.** The analyzer enforces immutable bindings for pointer writes.

---

## 11.8 Arrays

Array types are pointers with an element type and optional length:

```ag
T[]         # unsized array
T[N]        # array with a statically known length
```

```ag
let fixed: int[3] = [1, 2, 3];
let unsized: int[] = [4, 5, 6];
let bytes: char[4] = ['a', 'b', 'c', 0];
```

An array literal `[a, b, c]` allocates a block on the heap and returns a pointer
(there is no stack array literal). Elements are packed using their storage
width, so `[1u8, 2u8]` uses two bytes while `[1, 2]` uses two 8-byte slots. The
inferred type remembers the length so `for` can iterate it.

```ag
let values = [10, 20, 30];

for value in values {
    println(value);
}

for index, value in values {
    println(index);
    println(value);
}
```

`T[]` and `T*` share the same representation. The length in `T[N]` is metadata
used by iteration, not a runtime check.

Taking `&arr[i]` without a declared type infers a reference. To index through
the result as a pointer, state the pointer type explicitly:

```ag
var bytes: u8[4] = [1u8, 2u8, 3u8, 4u8];
var p: u8* = &bytes[0];
if (p[2] != 3) { return 1; }
```

> **Runs.** Fixed and unsized array types, literals, indexing, and `for`
> iteration work.

### Slices

Range indexing parses but is **not** a real slice:

```ag
let values: int[4] = [10, 20, 30, 40];
let part = values[1..3];        # parses
let x = part[0];                # currently reads values[0], not values[1]
```

> **Parse-only / incorrect.** Slicing does not adjust the base pointer or the
> length. Use an explicit offset pointer (`values + 1`) instead.

---

## 11.9 Strings as memory

`string` is a `char*` to a NUL-terminated byte buffer. String literals are
statically stored; `alloc`'d buffers are yours to free.

```ag
var buffer: char* = alloc(8);
buffer[0] = 'a';
buffer[1] = 'g';
buffer[2] = 0;

write(buffer, 2);       # "ag"
free(buffer);
```

For owned, resizable strings use `ag::String` (chapter 13).

---

## 11.10 Standard patterns

### Growable manual buffer

The language has no `realloc`; allocate a new block and copy with `memcpy`.

```ag
func grow(old: char*, old_size: int, new_size: int) -> char* {
    let fresh: char* = alloc(new_size);
    memcpy(fresh, old, old_size);
    free(old);
    return fresh;
}
```

### Out-parameters

```ag
func divide(a: int, b: int, out: int*) -> bool {
    if (b == 0) {
        return false;
    }
    *out = a / b;
    return true;
}

var result = 0;
if (divide(10, 2, &result)) {
    println(result);        # 5
}
```

### Links

Pointers can build linked structures, but there is no automatic initialization:

```ag
struct Node {
    value: int,
    next: Node*,
}

let first: Node* = alloc(16);
first.value = 1;
first.next = nullptr;       # null pointer
```

> **Note.** Use `null` or `nullptr` for a null pointer. Both are typed as
> `void*`, so they convert to any pointer type but not to `int`. A reference
> binding also accepts the literal, but a null reference must not be read or
> compared with `null`.

---

## 11.11 Complete runnable example

```ag
# memory.ag
mod ag::io;
import ag::io { println };

func set_out(out: int*) {
    *out = 7;
}

func add_one(value: &mut int) {
    value += 1;
}

func read_ref(value: &int) -> int => value;

func main() -> int {
    var x: int = 1;
    let r: &int = &x;
    println(read_ref(r));           # 1

    add_one(&mut x);
    println(x);                     # 2

    var numbers: int* = alloc(24);
    numbers[0] = 10;
    numbers[1] = 20;
    numbers[2] = 30;
    println(numbers[0] + numbers[1] + numbers[2]);  # 60
    println(*(numbers + 1));                        # 20

    let tail = numbers + 2;
    println(*tail);                 # 30

    var pp: int** = alloc(8);
    *pp = numbers;
    println((*pp)[1]);              # 20

    set_out(numbers);
    println(numbers[0]);            # 7

    var bytes: char* = alloc(4);
    bytes[0] = 'a';
    bytes[1] = 'b';
    bytes[2] = 0;
    write(bytes, 2);                # "ab"
    write("\n", 1);

    free(bytes);
    free(pp);
    free(numbers);
    return 0;
}
```

```sh
build/agc memory.ag -I lib -o /tmp/memory
/tmp/memory
```

Next: [12 — Optionals and errors](12-optionals-errors.md).

# 14 — Concurrency and async

AGC has syntax for spawning tasks, async functions, awaiting, and channel
operations. There is currently **no scheduler, threading, or channel runtime**:
all of these forms execute synchronously or are ignored.

> **Status: parse/compile markers.** Concurrency syntax does not introduce
> parallel execution today.

---

## 14.1 `spawn`

```ag
spawn {
    # statements
}
```

At statement position, `spawn { ... }` parses the block as a task and then runs
it like an ordinary block, in place.

```ag
println("before");

spawn {
    println("inside");
}

println("after");
```

Output:

```text
before
inside
after
```

There is no task handle, no join, and no return value.

> **No-op (synchronous).** The block runs immediately on the current thread.

---

## 14.2 `async` functions

`async` is accepted before `func` and ignored.

```ag
async func fetch(value: int) -> int {
    return value * 2;
}
```

```ag
let a = fetch(5);       # ordinary call
```

An `async` function has the same signature and call behavior as a normal
function. It does not return a future or task.

> **No-op (marker).** `async` does not change code generation or execution.

---

## 14.3 `await`

`await` is accepted as a prefix expression and evaluates its operand normally.

```ag
async func fetch(value: int) -> int => value * 2;

let result = await fetch(5);        # 10
```

It may appear in expressions:

```ag
let total = await fetch(1) + await fetch(2);
```

> **No-op (synchronous).** `await` does not suspend or poll; it is the identity
> on the operand.

---

## 14.4 Channels

The `<-` operator has two spellings:

```ag
channel <- value        # infix send
value = <-channel       # prefix receive
```

Current behavior:

- A send evaluates the value and produces it as the expression result.
- A receive evaluates the channel operand and produces that value.
- Nothing is queued, blocked, or transferred.

```ag
let channel = 7;

channel <- 42;              # evaluates 42; channel is unchanged
let received = <-channel;   # evaluates channel: 7
println(received);          # 7
```

```ag
func send_example(channel: int) -> int {
    channel <- 1;
    return channel;         # still the original sentinel
}
```

There is no built-in `Channel<T>` type, `send`/`receive` method, close
operation, or buffering. If you need a named channel-like value, use an
ordinary variable or a struct:

```ag
struct Chan {
    id: int,
    capacity: int,
}

let c = Chan { id: 1, capacity: 16 };
c.id <- 5;                  # accepted, still a pass-through
```

> **No-op (pass-through).** `<-` does not block and does not communicate
> between tasks.

---

## 14.5 Synchronization primitives

There are no compiler-provided mutexes, atomics, semaphores, or condition
variables. `examples/plan.ag` sketches imports such as `ag::sync`, but those
modules do not exist in the repository.

```ag
# Not available:
# import ag::sync { Channel, Mutex };
# import ag::thread;
```

> **Missing.** No threading or synchronization library ships with the compiler.

---

## 14.6 `move`

`move` may prefix a lambda or closure expression:

```ag
let callback = move (n: int) -> int => n + 1;
```

Lambdas are parsed, but first-class function values are not implemented, so the
resulting value cannot be called or stored usefully yet. `move` therefore has no
observable effect.

```ag
# Parses, but cannot be called:
let fn = move (n: int) -> int => n + 1;
# fn(1);                    # error
```

> **Parse-only.** Lambda and `move` syntax do not create callable values.

See chapter 15 for the current lambda limitations.

---

## 14.7 What concurrency looks like today

```ag
func worker(value: int) -> int {
    println(value);
    return value * 2;
}

func main() -> int {
    # Sequential, despite the syntax:
    spawn {
        println("task");
    }

    let result = await worker(5);
    println(result);

    let channel = 0;
    channel <- result;
    println(<-channel);

    return 0;
}
```

Output:

```text
task
5
10
10
```

Everything runs in source order.

---

## 14.8 Complete runnable example

```ag
# async.ag
mod ag::io;
import ag::io { println };

async func fetch(value: int) -> int {
    return value * 2;
}

func main() -> int {
    let a = await fetch(5);
    println(a);                 # 10

    let b = await fetch(1) + await fetch(2);
    println(b);                 # 6

    spawn {
        println(1);
    }
    println(2);                 # 1 then 2

    let channel = 7;
    channel <- 42;
    let received = <-channel;
    println(received);          # 7

    return 0;
}
```

```sh
build/agc async.ag -I lib -o /tmp/async
/tmp/async
```

Next: [15 — Attributes, compile-time markers, and missing features](15-misc.md).

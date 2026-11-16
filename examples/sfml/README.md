# SFML example

A 640×360 window with an animated rectangle, built with AGC and CSFML.

## Why there is a C shim

CSFML passes `sfVideoMode`, `sfColor`, and `sfVector2f` **by value**. AGC's
`extern "c"` currently passes struct values as pointers, so calling those
functions directly uses the wrong ABI. [`shim.c`](shim.c) provides scalar
wrappers that construct the SFML structs on the C side.

You do **not** need a shim for every SFML function. Functions whose signatures
use only pointers or plain scalars are declared directly in
[`src/main.ag`](src/main.ag) and called with `extern "c"`:

```ag
extern "c" {
  func sfRenderWindow_display(window: void*);
  func sfRenderWindow_destroy(window: void*);
  func sfRenderWindow_drawRectangleShape(window: void*, shape: void*,
                                         states: void*);
  func sfRectangleShape_destroy(shape: void*);
}
```

## Build and run

From this directory:

```sh
./build.sh
./build/sfml_demo
```

Or from the repository root:

```sh
examples/sfml/build.sh
examples/sfml/build/sfml_demo
```

Press **Escape** or use the window close button to exit.

The script compiles `shim.c` against the bundled headers, links the CSFML
libraries, and builds the AGC source in [`src/main.ag`](src/main.ag). If
`pkg-config` finds CSFML, system libraries are used; otherwise the bundled
copies in [`lib/`](lib) are used.

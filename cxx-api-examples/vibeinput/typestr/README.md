# typestr C library

A small C library extracted from robotgo that types UTF-8 strings, mirroring the behavior of the Go `TypeStr` function.

It reuses robotgo's existing C helpers (unicodeType, input_utf, microsleep) and supports macOS, Windows, and Linux/X11.

All required C and header files are vendored under this folder:

- `typestr/base/` (OS macros, types, sleep, X11 display, RNG)
- `typestr/key/` (keycodes and keypress helpers)

## API

```c
#include "typestr.h"

void typestr(const char* text, uintptr pid, int tm, int tm1, int8_t isPid);
void typestr_simple(const char* text);
```

- `text`: UTF-8 string to type
- `pid`: target window/process (0 = active application)
- `tm`: delay (ms) between typed characters
- `tm1`: extra delay (ms) used by X11 `input_utf` path (default 7)
- `isPid`: Windows only; when 1 treat `pid` as HWND; otherwise treat as process ID

## Build (CMake)

```bash
cmake -S typestr -B build/typestr
cmake --build build/typestr -j
```

On Linux/X11 you will need development packages for X11 and Xtst installed, e.g. on Debian/Ubuntu:

```bash
sudo apt-get install libx11-dev libxtst-dev
```

## Notes

- This library includes header-implemented sources from `key/keypress_c.h`, which define functions. If you link this library together with other robotgo C objects that also include those headers, you may get duplicate symbol errors. In that case, avoid linking this library into the same final binary, or refactor to compile those sources only once.
- The implementation uses a minimal UTF-8 decoder to convert the string into code points and feeds them to platform-specific event injection.

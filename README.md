# LeakCHK

LeakCHK is a simple, thread-safe leak checker that replaces the regular `stdlib.h` memory allocation functions with those, that track the number of allocations and the location where they happened.

## Usage

### Code

```c
#include <leakchk.h>

int main (void) {
    if (lc_init () != 0) {
        return -1;
    }

    void *p = lc_malloc (23);

    lc_deinit ();
    return 0;
}
```

### Output

```bash
$ gcc example.c -lleakchk
$ ./a.out
LEAKCHK: 1 tracked allocations have not been freed:
LC_BLOCK @0xFFFFF67923A9:
        size: 23, allocated in example.c:8 (function main)
```

## Building

To build the library on a \*NIX system clone the repository and it's submodules:

```bash
$ git clone https://github.com/slayers-git/leakchk.git
$ git submodule update --init
```

Then execute:

```bash
$ mkdir -p build && cd build 
$ cmake ../ && make
```

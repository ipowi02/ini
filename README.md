# ini.h - A simple INI parser in C.

Usage:
```c

#define INI_IMPLEMENTATION
#include "ini.h"

int main() {
  FILE *f = fopen("foo.ini", "rw");
  struct ini i = {0};
  if (ini(&i, f) < 0) {
    // handle error
  }
}
```
TODO

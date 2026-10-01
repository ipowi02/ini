# ini.h - A simple INI parser in C.

Usage:
```c
#define INI_IMPLEMENTATION
#include "ini.h"

int main(void)
{
    FILE *f = fopen("test.ini", "r");
    if (!f)
        return 1;

    struct ini_parser ini;
    struct ini_line pair;

    ini_init(&ini, f);

    while (ini_next(&ini, &pair) == INI_OK) {
        printf("[%s] %s = %s\n",
               pair.section,
               pair.key,
               pair.val);
    }

    fclose(f);
}
```
How do i write a readme

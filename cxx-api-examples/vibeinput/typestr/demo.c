#include "typestr.h"
#include "base/microsleep.h"
#include <stdio.h>

int main(void) {
    printf("typestr demo: will type after 10 seconds...\n");
    fflush(stdout);

    // Delay 10 seconds
    microsleep(10000.0);

    // Type UTF-8 string into active window
    typestr("hello from typestr,中文测试", 0, 0, 7, 0);

    printf("\nDone.\n");
    return 0;
}

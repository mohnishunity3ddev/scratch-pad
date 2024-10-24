#ifndef MEMORY_COMMON_H
#define MEMORY_COMMON_H

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool
is_power_of_2(uintptr_t x)
{
    bool result = ((x & (x - 1)) == 0);
    return result;
}

uintptr_t
align_forward(uintptr_t ptr, size_t align)
{
    uintptr_t p, a, modulo;
    assert(is_power_of_2(align));

    p = ptr;
    a = (uintptr_t)align;
    // p % a since a is a power of 2.
    modulo = (p & (a - 1));

    if (modulo != 0)
    {
        // p + (a-modulo) is divisible by p
        p += (a - modulo);
    }

    return p;
}

#endif
#ifndef HASH_HELPERS_H
#define HASH_HELPERS_H

#include <containers/string_utils.h>
#include <string.h>

unsigned int
string_hash(const char *str, unsigned int seed)
{
    size_t len = strnlen_s(str, UINT_MAX);
    unsigned int result = seed;

    for (size_t i = 0; i < len; ++i) {
        char c = str[i];
        result ^= c;
        result *= 16777619;
    }

    return result;
}

unsigned int
string_hash_ignore_case(const char *str, unsigned int seed)
{
    size_t len = strnlen_s(str, UINT_MAX);
    unsigned int result = seed;

    for (size_t i = 0; i < len; ++i) {
        char c = char_to_lower(str[i]);
        result ^= c;
        result *= 16777619;
    }

    return result;
}

unsigned int
float_hash(float f, unsigned int seed)
{
    unsigned int result = *(unsigned int *)&f;
    return result;
}

unsigned int
double_hash(double d, unsigned int seed)
{
    unsigned long long dbits = *(unsigned long long *)&d;

    unsigned int low = (unsigned int)(dbits & 0xFFFFFFFF);
    unsigned int high = (unsigned int)((dbits>>32) & 0xFFFFFFFF);

    unsigned int result = low ^ high;
    return result;
}

#endif
#ifndef COMMON_H
#define COMMON_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdint.h>
#include <string.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifdef SINT32_MAX
#error "This should not be defined"
#undef SINT32_MAX
#endif

/*
#if defined(_MSC_VER)
#define restrict __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define restrict __restrict__
#else
#define restrict
#endif
*/

#define SINT32_MAX 2147483647

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define GET_FORMAT_STR(x) _Generic((x), \
    char: "%c", \
    signed char: "%hhd", \
    unsigned char: "%hhu", \
    short: "%hd", \
    unsigned short: "%hu", \
    int: "%d", \
    unsigned int: "%u", \
    long: "%ld", \
    unsigned long: "%lu", \
    long long: "%lld", \
    unsigned long long: "%llu", \
    float: "%f", \
    double: "%f", \
    long double: "%Lf", \
    char*: "%s", \
    void*: "%p", \
    const char*: "%s", \
    const void*: "%p", \
    default: "%p")
#define print_type(x)                                                                                             \
    _Generic((x), int: "int", float: "float", double: "double", char *: "string", default: "unknown")

#else
#define GET_FORMAT_STR(x)
#define print_type(x)
#endif

#define TEST_ASSERT(condition, message)                                                                           \
    do                                                                                                            \
    {                                                                                                             \
        if (!(condition))                                                                                         \
        {                                                                                                         \
            printf("FAILED: %s (line %d)\n", message, __LINE__);                                                  \
            assert(condition);                                                                                    \
        }                                                                                                         \
    } while (0)

#endif
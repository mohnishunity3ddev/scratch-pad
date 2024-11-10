#ifndef COMMON_H
#define COMMON_H

#define max(a,b) (((a) > (b)) ? (a) : (b))

#ifdef SINT32_MAX
#error "This should not be defined"
#undef SINT32_MAX
#endif

#define SINT32_MAX 2147483647
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

#endif
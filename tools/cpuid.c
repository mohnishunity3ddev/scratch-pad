#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <cpuid.h>


void cpuid(int info[4], int eax, int ecx) {
    __cpuid_count(eax, ecx, info[0], info[1], info[2], info[3]);
}


bool supports_xsave() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 27)) != 0;
}


bool supports_os_avx() {
    if (!supports_xsave()) return false;

    unsigned int eax, edx;
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
    return (eax & 0x6) == 0x6;
}

bool
supports_os_avx__2()
{
    unsigned long long xcr0;
    __asm__("xgetbv" : "=A"(xcr0) : "c"(0) : "%edx");
    return (xcr0 & 6) == 6;
}


bool check_sse() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[3] & (1 << 25)) != 0;
}


bool check_sse2() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[3] & (1 << 26)) != 0;
}


bool check_sse3() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 0)) != 0;
}


bool check_sse41() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 19)) != 0;
}


bool check_sse42() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 20)) != 0;
}


bool check_avx() {
    int info[4];
    cpuid(info, 1, 0);
    return supports_os_avx() && (info[2] & (1 << 28)) != 0;
}


bool check_avx2() {
    int info[4];
    cpuid(info, 7, 0);
    return supports_os_avx__2() && (info[1] & (1 << 5)) != 0;
}


bool check_avx512f() {
    int info[4];
    cpuid(info, 7, 0);
    return supports_os_avx() && (info[1] & (1 << 16)) != 0;
}

bool check_avx512dq() {
    int info[4];
    cpuid(info, 7, 0);
    return supports_os_avx() && (info[1] & (1 << 17)) != 0;
}


bool check_avx512bw() {
    int info[4];
    cpuid(info, 7, 0);
    return supports_os_avx() && (info[1] & (1 << 30)) != 0;
}


bool check_avx512vl() {
    int info[4];
    cpuid(info, 7, 0);
    return supports_os_avx() && (info[1] & (1 << 31)) != 0;
}

bool strcmpb(const void *dst, const void *src, size_t len) {
    const unsigned char *d = (const unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < len; i++) {
        if (d[i] != s[i]) {
            return false;
        }
    }

    return true;
}


int
main(int argc, char **argv)
{
#ifdef __clang__
    char *arg = argv[1];
    size_t len = strlen(arg);
    printf("%s\n", arg);
    if (len == strlen("sse") && strcmpb(arg, "sse", strlen(arg))) {
        return !check_sse();
    } else if (len == strlen("sse2") && strcmpb(arg, "sse2", strlen(arg))) {
        return !check_sse2();
    } else if (len == strlen("sse3") && strcmpb(arg, "sse3", strlen(arg))) {
        return !check_sse3();
    } else if (len == strlen("sse41") && strcmpb(arg, "sse41", strlen(arg))) {
        return !check_sse41();
    } else if (len == strlen("sse42") && strcmpb(arg, "sse42", strlen(arg))) {
        return !check_sse42();
    } else if (len == strlen("avx") && strcmpb(arg, "avx", strlen(arg))) {
        return !check_avx();
    } else if (len == strlen("avx2") && strcmpb(arg, "avx2", strlen(arg))) {
        return !check_avx2();
    } else if (len == strlen("avx512") && strcmpb(arg, "avx512", strlen(arg))) {
        return !check_avx512f();
    } else if (len == strlen("avx512dq") && strcmpb(arg, "avx512dq", strlen(arg))) {
        return !check_avx512dq();
    } else if (len == strlen("avx512bw") && strcmpb(arg, "avx512bw", strlen(arg))) {
        return !check_avx512bw();
    } else if (len == strlen("avx512vl") && strcmpb(arg, "avx512vl", strlen(arg))) {
        return !check_avx512vl();
    }
#else
#error "only for clang"
#endif

    return 1;
}

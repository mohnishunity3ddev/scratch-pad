#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#if defined(_MSC_VER)
#include <intrin.h> // For __cpuidex and _xgetbv on MSVC
#else
#include <cpuid.h>  // For __cpuid_count on GCC/Clang
#endif

// Helper function to execute CPUID
void cpuid(int info[4], int eax, int ecx) {
#if defined(_MSC_VER)
    __cpuidex(info, eax, ecx);
#else
    __cpuid_count(eax, ecx, info[0], info[1], info[2], info[3]);
#endif
}

// Function to check if XSAVE is supported (needed for AVX and beyond)
bool supports_xsave() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 27)) != 0; // Bit 27 of ECX
}

// Function to check if the OS has enabled AVX support via XGETBV
bool supports_os_avx() {
    if (!supports_xsave()) return false;

    unsigned int eax, edx;
#if defined(_MSC_VER)
    eax = (unsigned int)_xgetbv(0);
    edx = 0;
#else
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
#endif
    return (eax & 0x6) == 0x6; // Check if XMM and YMM state are enabled
}

// Function to check SSE (SSE1)
bool check_sse() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[3] & (1 << 25)) != 0; // Bit 25 of EDX
}

// Check SSE2
bool check_sse2() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[3] & (1 << 26)) != 0; // Bit 26 of EDX
}

// Check SSE3
bool check_sse3() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 0)) != 0; // Bit 0 of ECX
}

// Check SSE4.1
bool check_sse41() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 19)) != 0; // Bit 19 of ECX
}

// Check SSE4.2
bool check_sse42() {
    int info[4];
    cpuid(info, 1, 0);
    return (info[2] & (1 << 20)) != 0; // Bit 20 of ECX
}

// Check AVX
bool check_avx() {
    int info[4];
    cpuid(info, 1, 0);
    return supports_os_avx() && (info[2] & (1 << 28)) != 0; // Bit 28 of ECX
}

// Check AVX2
bool check_avx2() {
    int info[4];
    cpuid(info, 7, 0); // Leaf 7, sub-leaf 0
    return supports_os_avx() && (info[1] & (1 << 5)) != 0; // Bit 5 of EBX
}

// Check AVX-512F (Foundation)
bool check_avx512f() {
    int info[4];
    cpuid(info, 7, 0); // Leaf 7, sub-leaf 0
    return supports_os_avx() && (info[1] & (1 << 16)) != 0; // Bit 16 of EBX
}

// Check AVX-512DQ (Doubleword and Quadword)
bool check_avx512dq() {
    int info[4];
    cpuid(info, 7, 0); // Leaf 7, sub-leaf 0
    return supports_os_avx() && (info[1] & (1 << 17)) != 0; // Bit 17 of EBX
}

// Check AVX-512BW (Byte and Word)
bool check_avx512bw() {
    int info[4];
    cpuid(info, 7, 0); // Leaf 7, sub-leaf 0
    return supports_os_avx() && (info[1] & (1 << 30)) != 0; // Bit 30 of EBX
}

// Check AVX-512VL (Vector Length Extensions)
bool check_avx512vl() {
    int info[4];
    cpuid(info, 7, 0); // Leaf 7, sub-leaf 0
    return supports_os_avx() && (info[1] & (1 << 31)) != 0; // Bit 31 of EBX
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

// Main function to test all features
int
main(int argc, char **argv)
{
    char *arg = argv[1];
    size_t len = strlen(arg);

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

    return 1;
}

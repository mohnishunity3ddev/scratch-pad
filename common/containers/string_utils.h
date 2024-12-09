#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "memory/memory.h"
#include <stdio.h>

static alloc_api *alloc_api_global = {NULL};

typedef struct string32 string32;
struct string32
{
    union {
        char *mem;
        char sso_string[8];
    } data;

    unsigned int length;
    unsigned int capacity;
    bool is_sso;
};

void string32_set_global_allocator(alloc_api *api);

string32    string32_create(const char *c_str, const alloc_api *alloc_api_);
#ifdef MUTABLE_STRINGS
void        string32_append(string32 *s, const char * c_str, const alloc_api *alloc_api_);
void        string32_modify(string32 *s, const char *c_str, const alloc_api *alloc_api_);
#endif
string32     string32_dup_char(const string32 *s, const alloc_api *alloc_api_);
string32    *string32_dup(const string32 *s, const alloc_api *api);
void         string32_cstr_free(string32 *s, const alloc_api *alloc_api_);
string32     string32_to_lower(const string32 *s, const alloc_api *api);

unsigned int string32_hash(const string32 *str, unsigned int seed);
bool         string32_compare(const string32 *restrict s1, const string32 *restrict s2);
void         string32_to_string(const string32 *s, char *buffer, size_t max_length);

const char  *string32_cstr(const string32 *s);
bool         string32_is_null_or_empty(const string32 *s);

char        char_to_lower(char c);

void string32_set_global_allocator(alloc_api *api) { alloc_api_global = api; }

#ifdef STRING32_UNIT_TESTS
void string32_unit_tests();
#endif

#ifdef STRING32_IMPLEMENTATION
string32
string32_create(const char *c_str, const alloc_api *api)
{
    string32 str = {};
    size_t len = strnlen_s(c_str, UINT_MAX);
    if (len == UINT_MAX) {
        printf("crossing the max length threshold.\n");
        return str;
    }
    // null terminated string.
    ++len;
    if (len > 8) {
        if (api == NULL) api = alloc_api_global;
        str.data.mem = shalloc_arr(api, char, len);
        assert(str.data.mem != NULL);
        str.capacity = len;
        str.length = len - 1;
        str.is_sso = false;
        strcpy_s(str.data.mem, str.capacity, c_str);
    } else {
        strcpy_s(str.data.sso_string, sizeof(char *), c_str);
        str.is_sso = true;
        str.length = len - 1;
        str.capacity = 8;
    }
    return str;
}

#ifdef MUTABLE_STRINGS
void
string32_append(string32 *s, const char *c_str, const alloc_api *api)
{
    size_t lenA = s->length;
    size_t lenB = strnlen_s(c_str, UINT_MAX);
    size_t total_length = lenA + lenB + 1;
    assert(total_length <= UINT_MAX);
    if (total_length <= 8) {
        memcpy_s(((char *)s->data.sso_string + lenA), 8-lenA, c_str, lenB);
        s->length = total_length - 1;
        s->data.sso_string[lenA + lenB] = '\0';
    } else {
        if (api == NULL)
            api = alloc_api_global;
        void *memory = (void *)s->data.mem;
        if (s->is_sso) {
            memory = shalloc_arr(api,char,total_length);
            memcpy_s(memory, total_length, s->data.sso_string, lenA);
            s->is_sso = false;
            s->capacity = total_length;
        } else if (s->capacity < total_length) {
            memory = shrealloc(api, s->data.mem, total_length);
            s->capacity = total_length;
        }

        s->data.mem = (char *)memory;
        s->length = total_length - 1;
        memcpy_s((char *)(s->data.mem + lenA), total_length-lenA, c_str, lenB);
        s->data.mem[lenA + lenB] = '\0';
    }
}

void
string32_modify(string32 *s, const char *c_str, const alloc_api *api)
{
    size_t len = strnlen_s(c_str, UINT_MAX);
    if (len < UINT_MAX)
    {
        ++len; // null terminator
        if (len <= 8 && s->is_sso) {
            memcpy_s(s->data.sso_string, 8, c_str, 8);
            s->length = len - 1;
        } else {
            void *memory = s->data.mem;

            if (s->capacity < len)
            {
                if (api == NULL)
                    api = alloc_api_global;
                memory = shalloc(api, len);
                if (!s->is_sso) {
                    shfree(api, s->data.mem);
                }
                s->capacity = len;
            }

            s->data.mem = (char *)memory;
            s->length = len-1;
            s->is_sso = false;
            memcpy_s(s->data.mem, s->capacity, c_str, len - 1);
            s->data.mem[s->length] = '\0';
        }
    } else {
        printf("[string32_modify]: crossing the string length threshold!\n");
    }
}
#endif

string32
string32_dup_char(const string32 *s, const alloc_api *api)
{
    assert(s != NULL);
    string32 result;
    if (s->is_sso) {
        memcpy_s(&result, sizeof(string32), s, sizeof(string32));
    } else {
        assert(s->data.mem != NULL);
        if (api == NULL)
            api = alloc_api_global;

        result.data.mem = shalloc_arr(api, char, s->length+1);
        strcpy_s(result.data.mem, s->capacity, s->data.mem);
        result.capacity = s->capacity;
        result.is_sso = false;
        result.length = s->length;
    }

    return result;
}

string32 *
string32_dup(const string32 *s, const alloc_api *api)
{
    assert(s != NULL);
    string32* result = shalloc_t(api, string32);
    if (s->is_sso) {
        memcpy_s(result, sizeof(string32), s, sizeof(string32));
    } else {
        assert(s->data.mem != NULL);
        if (api == NULL)
            api = alloc_api_global;

        result->data.mem = shalloc_arr(api, char, s->length+1);
        strcpy_s(result->data.mem, s->capacity, s->data.mem);
        result->capacity = s->capacity;
        result->is_sso = false;
        result->length = s->length;
    }

    return result;
}

unsigned int
string32_hash(const string32 *str, unsigned int seed)
{
    unsigned int result = seed;
    const char *s = string32_cstr(str);

    for (size_t i = 0; i < str->length; ++i) {
        char c = s[i];
        result ^= c;
        result *= 16777619;
    }

    return result;
}

bool
string32_compare(const string32 *restrict s1, const string32 *restrict s2)
{
    bool result = (s1->length == s2->length) && (s1->is_sso == s2->is_sso);
    if (!result)
        return false;

    if (!s1->is_sso) {
        result = (s1->data.mem[0] == s2->data.mem[0]) &&
                 (strncmp(s1->data.mem, s2->data.mem, s1->length) == 0);
    } else {
        result = (s1->data.sso_string[0] == s2->data.sso_string[0]) &&
                 (strncmp(s1->data.sso_string, s2->data.sso_string, sizeof(char *)) == 0);
    }

    return result;
}

string32
string32_to_lower(const string32 *s, const alloc_api *api)
{
    if (api == NULL) {
        api = alloc_api_global;
    }

    string32 result = string32_dup_char(s, api);
    return result;
}

const char *
string32_cstr(const string32 *s)
{
    const char *cstr = s->is_sso ? s->data.sso_string : s->data.mem;
    return cstr;
}

void
string32_to_string(const string32 *s, char *buffer, size_t max_length)
{
    assert((s->length+1 <= max_length) && "Buffer Length not enough for this string");
    snprintf(buffer, max_length, "%s", string32_cstr(s));
}

bool
string32_is_null_or_empty(const string32 *s)
{
    return  (s->data.sso_string[0] == '\0') ||
            (s->data.mem == NULL);
}

void
string32_cstr_free(string32 *s, const alloc_api *api)
{
    if (api == NULL)
        api = alloc_api_global;

    if (!s->is_sso) {
        shfree(api, s->data.mem);
    }
    memset(s, 0, sizeof(*s));
}

void string32_free(string32 *s, const alloc_api *api) {
    if (s != NULL) {
        if (api == NULL)
            api = alloc_api_global;

        if (!s->is_sso) {
            shfree(api, s->data.mem);
        }
        shfree(api, s);
    }
}

char
char_to_lower(char c)
{
    return c - 'a';
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// UNIT TESTS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifdef STRING32_UNIT_TESTS
#include "memory/freelist_alloc.h"
#define MUTABLE_STRINGS
void
test_string32_basic_sso(alloc_api *api)
{
    printf("%-40s", "Running Basic SSO Tests...");

    // Test all possible SSO lengths (0-7)
    const char* sso_strings[] = {
        "",
        "1",
        "12",
        "123",
        "1234",
        "12345",
        "123456",
        "1234567"
    };

    for (size_t i = 0; i < sizeof(sso_strings)/sizeof(char*); i++) {
        string32 s = string32_create(sso_strings[i], api);
        assert(s.is_sso);
        assert(s.length == strlen(sso_strings[i]));
        assert(strcmp(string32_cstr(&s), sso_strings[i]) == 0);
        string32_cstr_free(&s, api);
    }

    // Test incremental building within SSO
    string32 s1 = string32_create("", api);
    const char* chars = "abcdefg";
    for (size_t i = 0; i < strlen(chars); i++) {
        char c[2] = {chars[i], '\0'};
        string32_append(&s1, c, api);
        assert(s1.is_sso);
        assert(s1.length == i + 1);
    }
    string32_cstr_free(&s1, api);

    // Test SSO with special characters
    const char* special_chars[] = {
        "\0",
        "\n",
        "",
        "\r",
        "\\",
        "\"",
        "'",
        "\x01"
    };

    for (size_t i = 0; i < sizeof(special_chars)/sizeof(char*); i++) {
        string32 s = string32_create(special_chars[i], api);
        assert(s.is_sso);
        assert(memcmp(string32_cstr(&s), special_chars[i], strlen(special_chars[i])) == 0);
        string32_cstr_free(&s, api);
    }
    printf("[PASSED]\n");
}

void
test_string32_sso_heap_transition(alloc_api *api)
{
    printf("%-40s", "Running SSO-Heap Transition Tests...");

    // Test exact boundary transitions
    string32 s1 = string32_create("1234567", api);
    assert(s1.is_sso);
    string32_append(&s1, "8", api);
    assert(!s1.is_sso);
    assert(strcmp(string32_cstr(&s1), "12345678") == 0);
    string32_cstr_free(&s1, api);

    // Test multiple transitions back and forth
    string32 s2 = string32_create("", api);
    const char* long_str = "This is a long string that won't fit in SSO";
    const char* short_str = "short";

    for (int i = 0; i < 100; i++) {
        string32_modify(&s2, (i % 2 == 0) ? long_str : short_str, api);
        assert(strcmp(string32_cstr(&s2), (i % 2 == 0) ? long_str : short_str) == 0);
    }
    string32_cstr_free(&s2, api);

    // Test gradual growth across boundary
    string32 s3 = string32_create("", api);
    char buffer[1024] = {0};
    for (size_t i = 0; i < 20; i++) {
        strcat(buffer, "X");
        string32_modify(&s3, buffer, api);
        assert(s3.is_sso == (i < 7));
        assert(strcmp(string32_cstr(&s3), buffer) == 0);
    }
    string32_cstr_free(&s3, api);
    printf("[PASSED]\n");
}

void
test_string32_comparison(alloc_api *api)
{
    printf("%-40s", "Running String Comparison Tests...");

    // Test various length combinations
    const char* test_strings[] = {
        "",
        "a",
        "ab",
        "abc",
        "abcd",
        "abcde",
        "abcdef",
        "abcdefg",
        "abcdefgh",
        "abcdefghi",
        "This is a much longer string for testing"
    };

    for (size_t i = 0; i < sizeof(test_strings)/sizeof(char*); i++) {
        for (size_t j = 0; j < sizeof(test_strings)/sizeof(char*); j++) {
            string32 s1 = string32_create(test_strings[i], api);
            string32 s2 = string32_create(test_strings[j], api);
            assert(string32_compare(&s1, &s2) == (strcmp(test_strings[i], test_strings[j]) == 0));
            string32_cstr_free(&s1, api);
            string32_cstr_free(&s2, api);
        }
    }

    // Test with special characters
    string32 special_pairs[][2] = {
        {string32_create("\0", api), string32_create("\0", api)},
        {string32_create("\n", api), string32_create("\n", api)},
        {string32_create("\t", api), string32_create("\t", api)},
        {string32_create("a\0b", api), string32_create("a\0b", api)},
        {string32_create("a\nb", api), string32_create("a\nb", api)}
    };

    for (size_t i = 0; i < sizeof(special_pairs)/(2*sizeof(string32)); i++) {
        assert(string32_compare(&special_pairs[i][0], &special_pairs[i][1]));
        string32_cstr_free(&special_pairs[i][0], api);
        string32_cstr_free(&special_pairs[i][1], api);
    }
    printf("[PASSED]\n");
}

void
test_string32_large_strings(alloc_api *api)
{
    printf("%-40s", "Running Large String Tests...");
    char *large_buffer = NULL;
    // Test with increasingly large strings
    large_buffer = shalloc_arr(api, char, 10000);
    memset(large_buffer, 'A', 9999);
    large_buffer[9999] = '\0';

    string32 s1 = string32_create("", api);
    for (size_t len = 8; len < 10000; len *= 2) {
        large_buffer[len] = '\0';
        string32_modify(&s1, large_buffer, api);
        assert(!s1.is_sso);
        assert(s1.length == len);
        assert(strncmp(string32_cstr(&s1), large_buffer, len) == 0);
        large_buffer[len] = 'A';
    }
    string32_cstr_free(&s1, api);

    // Test appending to large strings
    string32 s2 = string32_create(large_buffer, api);
    const char* append_str = "MORE";
    for (int i = 0; i < 100; i++) {
        string32_append(&s2, append_str, api);
        assert(!s2.is_sso);
    }
    string32_cstr_free(&s2, api);

    shfree(api, large_buffer);
    printf("[PASSED]\n");
}

void
test_string32_duplication(alloc_api *api)
{
    printf("%-40s", "Running String Duplication Tests...");

    // Test duplication of strings of various lengths
    const char* test_cases[] = {
        "",
        "a",
        "1234567",  // SSO boundary
        "12345678", // Just over SSO
        "This is a much longer string that won't fit in SSO buffer",
        "Even longer string with lots of content to ensure multiple allocations might be needed..."
    };

    for (size_t i = 0; i < sizeof(test_cases)/sizeof(char*); i++) {
        string32 original = string32_create(test_cases[i], api);
        string32 duplicate = string32_dup_char(&original, api);

        assert(string32_compare(&original, &duplicate));
        assert(original.length == duplicate.length);
        assert(original.is_sso == duplicate.is_sso);

        // Modify duplicate and verify original unchanged
        string32_append(&duplicate, "_modified", api);
        assert(!string32_compare(&original, &duplicate));
        assert(strcmp(string32_cstr(&original), test_cases[i]) == 0);

        string32_cstr_free(&original, api);
        string32_cstr_free(&duplicate, api);
    }
    printf("[PASSED]\n");
}

void
test_string32_modifications(alloc_api *api)
{
    printf("%-40s", "Running String Modification Tests...");

    // Test rapid modifications
    string32 s1 = string32_create("", api);
    const char* modifications[] = {
        "small",
        "larger string",
        "even larger string that won't fit in SSO",
        "tiny",
        "back to a huge string that requires heap allocation",
        "sso",
        "this is getting really long now and will need more space",
        "short",
        NULL
    };

    for (int iteration = 0; iteration < 100; iteration++) {
        for (const char** mod = modifications; *mod != NULL; mod++) {
            string32_modify(&s1, *mod, api);
            assert(strcmp(string32_cstr(&s1), *mod) == 0);
        }
    }
    string32_cstr_free(&s1, api);

    // Test alternating append and modify
    string32 s2 = string32_create("base", api);
    for (int i = 0; i < 100; i++) {
        if (i % 2 == 0) {
            string32_append(&s2, "_append", api);
        } else {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "mod_%d", i);
            string32_modify(&s2, buffer, api);
        }
    }
    string32_cstr_free(&s2, api);
    printf("[PASSED]\n");
}

void
test_string32_edge_cases(alloc_api *api)
{
    printf("%-40s", "Running Edge Case Tests...");

    // Test empty string operations
    string32 s1 = string32_create("", api);
    assert(string32_is_null_or_empty(&s1));

    for (int i = 0; i < 100; i++) {
        string32_append(&s1, "", api);
        assert(string32_is_null_or_empty(&s1));
    }
    string32_cstr_free(&s1, api);

    // Test with strings containing null bytes
    char null_str[] = "abc\0def";
    string32 s2 = string32_create(null_str, api);
    assert(s2.length == 3);  // Should only count up to null byte
    string32_cstr_free(&s2, api);

    // Test modification with same content
    string32 s3 = string32_create("test", api);
    for (int i = 0; i < 100; i++) {
        string32_modify(&s3, "test", api);
        assert(strcmp(string32_cstr(&s3), "test") == 0);
    }
    string32_cstr_free(&s3, api);
    printf("[PASSED]\n");
}

void
test_string32_stress(alloc_api *api)
{
    printf("%-40s", "Running Stress Tests...");

    // Create many strings simultaneously
    const int NUM_STRINGS = 1000;
    string32 *strings = shalloc_arr(api, string32, NUM_STRINGS);
    // Initialize with varying lengths
    for (int i = 0; i < NUM_STRINGS; i++) {
        char buffer[1024];
        int len = i % 20;  // Vary length from 0 to 19
        memset(buffer, 'X', len);
        buffer[len] = '\0';
        strings[i] = string32_create(buffer, api);
    }

    // Modify all strings multiple times
    for (int iter = 0; iter < 10; iter++) {
        for (int i = 0; i < NUM_STRINGS; i++) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), "iter_%d_str_%d", iter, i);
            string32_modify(&strings[i], buffer, api);
            assert(strcmp(string32_cstr(&strings[i]), buffer) == 0);
        }
    }

    // Cleanup
    for (int i = 0; i < NUM_STRINGS; i++) {
        string32_cstr_free(&strings[i], api);
    }
    shfree(api, strings);

    // Test rapid allocation/deallocation
    for (int i = 0; i < 1000; i++) {
        string32 s = string32_create("test", api);
        string32_modify(&s, "This is a longer string that won't fit in SSO", api);
        string32_modify(&s, "short again", api);
        string32_append(&s, "make it longer again with some appended text", api);
        string32_cstr_free(&s, api);
    }
    printf("[PASSED]\n");
}

void
string32_unit_tests()
{
    printf("Running Various String Unit Tests...\n\n");

    Freelist fl;
    size_t mem_size = 1024 * 1024;
    void *memory = malloc(mem_size);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;
    freelist_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);
    alloc_api *api = &fl.api;

    // Run all test categories
    test_string32_basic_sso(api);
    test_string32_sso_heap_transition(api);
    test_string32_comparison(api);
    test_string32_large_strings(api);
    test_string32_duplication(api);
    test_string32_modifications(api);
    test_string32_edge_cases(api);
    test_string32_stress(api);
    printf("\nAll tests completed successfully!\n");

    freelist_free_all(&fl);
    free(memory);
}
#endif

#endif
#endif
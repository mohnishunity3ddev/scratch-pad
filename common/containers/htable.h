#ifndef HTABLE_H
#define HTABLE_H

#include <common.h>
#include <memory/memory.h>
#ifndef STRING32_IMPLEMENTATION
#define STRING32_IMPLEMENTATION
#endif
#include <containers/string_utils.h>
#include <stdint.h>

#define HTABLE_LOAD_FACTOR_CHECK(ht) (float)((ht)->count + 1) / (float)((ht)->capacity)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define DEFAULT_FUNCS(T, name)                                                                                    \
    inline bool name##_compare(T a, T b) { return a == b; }                                                       \
    inline T name##_dup(const T k, const alloc_api *api) { return k; }                                            \
    inline void name##_free(T k, const alloc_api *api) {}                                                         \
    inline void name##_to_string(T x, char *buffer, size_t max_len)                                               \
    {                                                                                                             \
        snprintf(buffer, max_len, GET_FORMAT_STR(x), x);                                                          \
    }
#else
#define DEFAULT_FUNCS(T, name)                                                                                    \
    inline bool name##_compare(T a, T b) { return a == b; }                                                       \
    inline T name##_dup(const T k, const alloc_api *api) { return k; }                                            \
    inline void name##_free(T k, const alloc_api *api) {}                                                         \
    inline void name##_to_string(T x, char *buffer, size_t max_len) {}

#endif
DEFAULT_FUNCS(float, float)

#define HTABLE_API(tkey, tval, name)                                                                              \
    typedef bool (*key_comparator_func_##name##_t)(const tkey a, const tkey b);                                   \
    typedef unsigned int (*key_hash_func_##name##_t)(const tkey key, unsigned int seed);                          \
    typedef tkey (*key_copy_func_##name##_t)(const tkey key, const alloc_api *api);                               \
    typedef void (*key_free_func_##name##_t)(tkey key, const alloc_api *api);                                     \
    typedef void (*key_display_func_##name##_t)(const tkey key, char *buffer, size_t max_len);                    \
    typedef tval (*value_copy_func_##name##_t)(const tval value, const alloc_api *api);                           \
    typedef void (*value_free_func_##name##_t)(tval value, const alloc_api *api);                                 \
    typedef void (*value_display_func_##name##_t)(const tval value, char *buffer, size_t max_len);                \
                                                                                                                  \
    typedef struct htable_functions_##name                                                                        \
    {                                                                                                             \
        key_comparator_func_##name##_t key_comparator_func;                                                       \
        key_hash_func_##name##_t key_hash_func;                                                                   \
        key_copy_func_##name##_t key_copy_func;                                                                   \
        key_free_func_##name##_t key_free_func;                                                                   \
        key_display_func_##name##_t key_display_func;                                                             \
        value_copy_func_##name##_t value_copy_func;                                                               \
        value_free_func_##name##_t value_free_func;                                                               \
        value_display_func_##name##_t value_display_func;                                                         \
    } htable_functions_##name;                                                                                    \
                                                                                                                  \
    typedef struct htable_key_##name                                                                              \
    {                                                                                                             \
        tkey key;                                                                                                 \
        unsigned int hash;                                                                                        \
    } htable_key_##name;                                                                                          \
                                                                                                                  \
    typedef struct htable_entry_##name                                                                            \
    {                                                                                                             \
        htable_key_##name ht_key;                                                                                 \
        tval value;                                                                                               \
    } htable_entry_##name;                                                                                        \
                                                                                                                  \
    typedef struct htable_##name                                                                                  \
    {                                                                                                             \
        size_t capacity;                                                                                          \
        size_t count;                                                                                             \
        htable_entry_##name *entries;                                                                             \
        tkey tombstone;                                                                                           \
                                                                                                                  \
        htable_functions_##name funcs;                                                                            \
                                                                                                                  \
        const alloc_api *api;                                                                                     \
        unsigned int seed;                                                                                        \
        float max_load_factor;                                                                                    \
        float min_load_factor;                                                                                    \
    } htable_##name;                                                                                              \
                                                                                                                  \
    void ht_init_##name(htable_##name *ht, size_t initial_capacity, float min_load_factor, float max_load_factor, \
                        unsigned int seed, const alloc_api *api);                                                 \
    void ht_add_##name(htable_##name *ht, const tkey key, tval value);                                            \
    bool ht_get_##name(const htable_##name *ht, const tkey key, tval *value);                                     \
    bool ht_key_exists_##name(const htable_##name *ht, const tkey key);                                           \
    void ht_resize_##name(htable_##name *ht, size_t new_capacity);                                                \
    bool ht_remove_key_##name(htable_##name *ht, const tkey key_to_remove);                                       \
    void ht_clear_##name(htable_##name *ht);                                                                      \
    void ht_delete_##name(htable_##name *ht)

#define htinit(name,pTable,cap,LF,maxLF,seed,api) ht_init_##name(pTable,cap,LF,maxLF,seed,api)
#define htpush(name,pTable,k,v) ht_add_##name(pTable,k,v)
#define htget(name,pTable,k,v) ht_get_##name(pTable,k,v)
#define htkeyexists(name,pTable,k) ht_key_exists_##name(pTable,k)
#define htresize(name,pTable,newCap) ht_resize_##name(pTable,newCap)
#define htdel(name,pTable,k) ht_remove_key_##name(pTable,k)
#define htclear(name,pTable) ht_clear_##name(pTable)
#define htdestroy(name, pTable) ht_delete_##name(pTable)

DEFAULT_FUNCS(uintptr_t, uintptr)
HTABLE_API(uintptr_t, uintptr_t, ptr_ptr);

#ifdef HASHTABLE_IMPLEMENTATION
#define HTABLE_API_IMPL(tkey, tval, tkey_name, tval_name, name)                                                   \
    inline void ht_init_##name(htable_##name *ht, size_t initial_capacity, float min_load_factor,                 \
                               float max_load_factor, unsigned int seed, const alloc_api *api)                    \
    {                                                                                                             \
        memset(ht, 0, sizeof(htable_##name));                                                                     \
                                                                                                                  \
        if (initial_capacity == 0)                                                                                \
        {                                                                                                         \
            initial_capacity = 2;                                                                                 \
        }                                                                                                         \
        else if (initial_capacity >= 65536)                                                                       \
        {                                                                                                         \
            printf("this is too large. Are you sure?\n");                                                         \
        }                                                                                                         \
        ht->capacity = initial_capacity;                                                                          \
        ht->count = 0;                                                                                            \
        ht->seed = seed;                                                                                          \
        ht->min_load_factor = min_load_factor;                                                                    \
        ht->max_load_factor = max_load_factor;                                                                    \
        ht->api = api;                                                                                            \
        ht->tombstone = (tkey)2;                                                                                  \
        ht->funcs.key_comparator_func = tkey_name##_compare;                                                      \
        ht->funcs.key_copy_func = tkey_name##_dup;                                                                \
        ht->funcs.key_free_func = tkey_name##_free;                                                               \
        ht->funcs.key_hash_func = tkey_name##_hash;                                                               \
        ht->funcs.key_display_func = tkey_name##_to_string;                                                       \
        ht->funcs.value_display_func = tval_name##_to_string;                                                     \
        ht->funcs.value_free_func = tval_name##_free;                                                             \
        ht->funcs.value_copy_func = tval_name##_dup;                                                              \
        ht->entries = shalloc_arr(ht->api, htable_entry_##name, initial_capacity);                                \
        memset(ht->entries, 0, sizeof(htable_entry_##name) * initial_capacity);                                   \
    }                                                                                                             \
                                                                                                                  \
    static inline bool ht_find_entry_##name(const htable_##name *ht, const tkey key_to_find,                      \
                                            size_t *out_entry_index)                                              \
    {                                                                                                             \
        unsigned int hash = ht->funcs.key_hash_func(key_to_find, ht->seed);                                       \
        unsigned int index = hash & (ht->capacity - 1);                                                           \
        htable_entry_##name *curr = ht->entries + index;                                                          \
        htable_entry_##name *iter = curr;                                                                         \
        bool found = false;                                                                                       \
        if (iter->ht_key.key != (tkey)0)                                                                          \
        {                                                                                                         \
            do                                                                                                    \
            {                                                                                                     \
                tkey key = iter->ht_key.key;                                                                      \
                if (key == (tkey)0)                                                                               \
                    break;                                                                                        \
                if (key != ht->tombstone && ht->funcs.key_comparator_func(key, key_to_find))    \
                {                                                                                                 \
                    found = true;                                                                                 \
                    if (out_entry_index != NULL)                                                                  \
                    {                                                                                             \
                        *out_entry_index = index;                                                                 \
                    }                                                                                             \
                    break;                                                                                        \
                }                                                                                                 \
                index = (index + 1) & (ht->capacity - 1);                                                         \
                iter = ht->entries + index;                                                                       \
            } while (iter != curr);                                                                               \
        }                                                                                                         \
        return found;                                                                                             \
    }                                                                                                             \
                                                                                                                  \
    static inline float ht_load_factor_##name(const htable_##name *ht)                                            \
    {                                                                                                             \
        float lf = (float)ht->count / (float)ht->capacity;                                                        \
        return lf;                                                                                                \
    }                                                                                                             \
                                                                                                                  \
    inline void ht_add_##name(htable_##name *ht, const tkey key, tval value)                                      \
    {                                                                                                             \
        if (HTABLE_LOAD_FACTOR_CHECK(ht) > ht->max_load_factor)                                                   \
        {                                                                                                         \
            if (ht->capacity == 0)                                                                                \
            {                                                                                                     \
                ht_resize_##name(ht, 2);                                                                          \
            }                                                                                                     \
            else                                                                                                  \
            {                                                                                                     \
                ht_resize_##name(ht, ht->capacity * 2);                                                           \
            }                                                                                                     \
        }                                                                                                         \
        unsigned int hash = ht->funcs.key_hash_func(key, ht->seed);                                               \
        unsigned int index = hash & (ht->capacity - 1);                                                           \
        htable_entry_##name *entry = ht->entries + index;                                                         \
        htable_entry_##name *start_entry = entry;                                                                 \
        tkey k = entry->ht_key.key;                                                                               \
        bool found = false;                                                                                       \
        do                                                                                                        \
        {                                                                                                         \
            if (k == (tkey)0 || k == ht->tombstone)                                                               \
            {                                                                                                     \
                break;                                                                                            \
            }                                                                                                     \
            if (ht->funcs.key_comparator_func(k, key))                                                            \
            {                                                                                                     \
                found = true;                                                                                     \
                break;                                                                                            \
            }                                                                                                     \
            index = (index + 1) & (ht->capacity - 1);                                                             \
            entry = ht->entries + index;                                                                          \
            k = entry->ht_key.key;                                                                                \
        } while (entry != start_entry);                                                                           \
        if (!found)                                                                                               \
        {                                                                                                         \
            htable_key_##name *hkey = &entry->ht_key;                                                             \
            hkey->key = ht->funcs.key_copy_func(key, ht->api);                                                    \
            hkey->hash = hash;                                                                                    \
            entry->ht_key = *hkey;                                                                                \
            ++ht->count;                                                                                          \
        }                                                                                                         \
        else                                                                                                      \
        {                                                                                                         \
            ht->funcs.value_free_func(entry->value, ht->api);                                                     \
        }                                                                                                         \
        entry->value = ht->funcs.value_copy_func(value, ht->api);                                                 \
    }                                                                                                             \
                                                                                                                  \
    inline bool ht_get_##name(const htable_##name *ht, const tkey key_to_search, tval *out_value_ptr)             \
    {                                                                                                             \
        size_t index = 0;                                                                                         \
        bool found = ht_find_entry_##name(ht, key_to_search, &index);                                             \
        htable_entry_##name *entry = ht->entries + index;                                                         \
        if (found && out_value_ptr != NULL)                                                                       \
        {                                                                                                         \
            *out_value_ptr = entry->value;                                                                        \
        }                                                                                                         \
        return found;                                                                                             \
    }                                                                                                             \
                                                                                                                  \
    inline bool ht_key_exists_##name(const htable_##name *ht, const tkey key)                                     \
    {                                                                                                             \
        return ht_get_##name(ht, key, NULL);                                                                      \
    }                                                                                                             \
                                                                                                                  \
    inline void ht_add_rehash_##name(htable_##name *ht, const htable_key_##name *key, tval value)                 \
    {                                                                                                             \
        unsigned int index = key->hash & (ht->capacity - 1);                                                      \
        htable_entry_##name *entry = ht->entries + index;                                                         \
        tkey k = entry->ht_key.key;                                                                               \
        while (k != (tkey)0 && k != ht->tombstone)                                                                \
        {                                                                                                         \
            index = (index + 1) & (ht->capacity - 1);                                                             \
            entry = ht->entries + index;                                                                          \
            k = entry->ht_key.key;                                                                                \
        }                                                                                                         \
        entry->ht_key.key = key->key;                                                                             \
        entry->ht_key.hash = key->hash;                                                                           \
        entry->value = value;                                                                                     \
                                                                                                                  \
        ++ht->count;                                                                                              \
    }                                                                                                             \
                                                                                                                  \
    inline void ht_resize_##name(htable_##name *ht, size_t new_capacity)                                          \
    {                                                                                                             \
        htable_entry_##name *old_entries = ht->entries;                                                           \
        size_t old_count = ht->count;                                                                             \
        size_t old_cap = ht->capacity;                                                                            \
        ht->capacity = new_capacity;                                                                              \
        ht->count = 0;                                                                                            \
        ht->entries = shalloc_arr(ht->api, htable_entry_##name, new_capacity);                                    \
        memset(ht->entries, 0, sizeof(htable_entry_##name) * ht->capacity);                                       \
        if (old_entries != NULL)                                                                                  \
        {                                                                                                         \
            for (size_t i = 0; i < old_cap; ++i)                                                                  \
            {                                                                                                     \
                htable_entry_##name *entry = old_entries + i;                                                     \
                tkey key = entry->ht_key.key;                                                                     \
                if (key != (tkey)0 && key != ht->tombstone)                                                       \
                {                                                                                                 \
                    ht_add_rehash_##name(ht, &entry->ht_key, entry->value);                                       \
                }                                                                                                 \
            }                                                                                                     \
            assert(ht->count == old_count);                                                                       \
            shfree(ht->api, old_entries);                                                                         \
        }                                                                                                         \
    }                                                                                                             \
                                                                                                                  \
    inline bool ht_remove_key_##name(htable_##name *ht, const tkey key_to_remove)                                 \
    {                                                                                                             \
        size_t index = 0xffffffffffffffffULL;                                                                     \
        bool found = ht_find_entry_##name(ht, key_to_remove, &index);                                             \
        if (!found)                                                                                               \
        {                                                                                                         \
            char buffer[128];                                                                                     \
            ht->funcs.key_display_func(key_to_remove, buffer, sizeof(buffer));                                    \
            printf("The key '%s' is not present in the hashtable.\n", buffer);                                    \
            return false;                                                                                         \
        }                                                                                                         \
        assert(index != 0xffffffffffffffffULL);                                                                   \
        htable_entry_##name *entry = ht->entries + index;                                                         \
        ht->funcs.key_free_func(entry->ht_key.key, ht->api);                                                      \
        ht->funcs.value_free_func(entry->value, ht->api);                                                         \
        entry->ht_key.key = ht->tombstone;                                                                        \
        entry->value = 0;                                                                                         \
        ht->count--;                                                                                              \
        if (ht_load_factor_##name(ht) <= ht->min_load_factor)                                                     \
        {                                                                                                         \
            ht_resize_##name(ht, ht->capacity / 2);                                                               \
        }                                                                                                         \
        return found;                                                                                             \
    }                                                                                                             \
                                                                                                                  \
    inline void ht_display_##name(const htable_##name *ht)                                                        \
    {                                                                                                             \
        printf("Hash Table:\n");                                                                                  \
        for (size_t i = 0; i < ht->capacity; ++i)                                                                 \
        {                                                                                                         \
            htable_entry_##name *entry = ht->entries + i;                                                         \
            tkey key = entry->ht_key.key;                                                                         \
            if (key != (tkey)0 && key != ht->tombstone)                                                           \
            {                                                                                                     \
                char buffer[128];                                                                                 \
                ht->funcs.key_display_func(key, buffer, sizeof(buffer));                                          \
                printf("[\"%s\"]:= ", buffer);                                                                    \
                                                                                                                  \
                ht->funcs.value_display_func(entry->value, buffer, sizeof(buffer));                               \
                printf("%s.\n", buffer);                                                                          \
            }                                                                                                     \
        }                                                                                                         \
    }                                                                                                             \
                                                                                                                  \
    inline void ht_clear_##name(htable_##name *ht)                                                                \
    {                                                                                                             \
        for (size_t i = 0; i < ht->capacity; ++i)                                                                 \
        {                                                                                                         \
            tkey key = ht->entries[i].ht_key.key;                                                                 \
            if (key != (tkey)0 && key != ht->tombstone)                                                           \
            {                                                                                                     \
                ht->funcs.key_free_func(key, ht->api);                                                            \
                ht->funcs.value_free_func(ht->entries[i].value, ht->api);                                         \
            }                                                                                                     \
        }                                                                                                         \
        memset(ht->entries, 0, sizeof(htable_entry_##name) * ht->capacity);                                       \
        ht->count = 0;                                                                                            \
    }                                                                                                             \
                                                                                                                  \
    inline void ht_delete_##name(htable_##name *ht)                                                               \
    {                                                                                                             \
        ht_clear_##name(ht);                                                                                      \
        shfree(ht->api, ht->entries);                                                                             \
        ht->entries = NULL;                                                                                       \
        ht->capacity = 0;                                                                                         \
    }
#define HTABLE_API_IMPL_PTR(TKey, TVal, name) HTABLE_API_IMPL(TKey*, TVal*, TKey, TVal, name)

#include <stdio.h>
inline unsigned int
uintptr_hash(const uintptr_t n, unsigned int seed)
{
    unsigned int result = seed;
    result ^= n;
    result *= 16777619;
    return result;
}

HTABLE_API_IMPL(uintptr_t, uintptr_t, uintptr, uintptr, ptr_ptr)

#ifdef HASHTABLE_UNIT_TESTS
#include <float.h>
#include <math.h>

#include <hash_helpers.h>

HTABLE_API(string32 *, float, str_float);
HTABLE_API(string32 *, string32 *, str_str);
HTABLE_API_IMPL(string32 *, float, string32, float, str_float)
HTABLE_API_IMPL_PTR(string32, string32, str_str)

static void
test_hashtable_simple(Freelist *fl)
{
    const alloc_api *api = &fl->api;
    string32 k1 = string32_create("Hello", api);
    string32 k2 = string32_create("There", api);
    string32 k3 = string32_create("Universe", api);
    string32 k4 = string32_create("PI", api);
    string32 k5 = string32_create("Mani", api);
    string32 k6 = string32_create("Mohnish", api);
    string32 k7 = string32_create("Sharma", api);
    string32 k8 = string32_create("Ayushi Sharma Kahan hai tu!", api);

    size_t used1 = fl->used;

    htable_str_float table;
    ht_init_str_float(&table, 16, 0.2f, 0.7f, 31, api);

    size_t used2 = fl->used;

    ht_add_str_float(&table, &k1, 1.0f);
    ht_add_str_float(&table, &k2, 2.0f);
    ht_add_str_float(&table, &k3, 3.0f);
    // ht_display_str_float(&table);

    ht_add_str_float(&table, &k4, 3.14159f);
    // ht_display_str_float(&table);
    ht_add_str_float(&table, &k5, 5.0f);
    ht_add_str_float(&table, &k6, 6.0f);
    ht_add_str_float(&table, &k7, 7.0f);
    ht_add_str_float(&table, &k8, 8.0f);
    // ht_display_str_float(&table);

    ht_clear_str_float(&table);
    ht_add_str_float(&table, &k1, 1.0f);
    ht_add_str_float(&table, &k2, 2.0f);
    ht_add_str_float(&table, &k3, 3.0f);
    ht_add_str_float(&table, &k4, 3.14159f);
    ht_add_str_float(&table, &k5, 5.0f);
    ht_add_str_float(&table, &k6, 6.0f);
    ht_add_str_float(&table, &k7, 7.0f);
    ht_add_str_float(&table, &k8, 8.0f);
    // ht_display_str_float(&table);

    ht_delete_str_float(&table);
    ht_add_str_float(&table, &k1, 1.0f);
    ht_add_str_float(&table, &k2, 2.0f);
    ht_add_str_float(&table, &k3, 3.0f);
    ht_add_str_float(&table, &k4, 3.14159f);
    ht_add_str_float(&table, &k5, 5.0f);
    ht_add_str_float(&table, &k6, 6.0f);
    ht_add_str_float(&table, &k7, 7.0f);
    ht_add_str_float(&table, &k8, 8.0f);
    // ht_display_str_float(&table);

    float val = 0.0f;
    assert(ht_get_str_float(&table, &k1, &val));
    assert(val == 1.0f);
    assert(ht_get_str_float(&table, &k2, &val));
    assert(val == 2.0f);
    assert(ht_get_str_float(&table, &k3, &val));
    assert(val == 3.0f);
    assert(ht_get_str_float(&table, &k4, &val));
    assert(val == 3.14159f);
    assert(ht_get_str_float(&table, &k5, &val));
    assert(val == 5.0f);
    assert(ht_get_str_float(&table, &k6, &val));
    assert(val == 6.0f);
    assert(ht_get_str_float(&table, &k7, &val));
    assert(val == 7.0f);
    assert(ht_get_str_float(&table, &k8, &val));
    assert(val == 8.0f);
    string32 not_present = string32_create("Not there!", api);
    assert(ht_get_str_float(&table, &not_present, NULL) == false);
    assert(ht_key_exists_str_float(&table, &not_present) == false);
    string32_free(&not_present, api);

    assert(ht_remove_key_str_float(&table, &k1));
    // ht_display_str_float(&table);
    assert(ht_remove_key_str_float(&table, &k2));
    // ht_display_str_float(&table);
    assert(ht_remove_key_str_float(&table, &k3));
    // ht_display_str_float(&table);
    assert(ht_remove_key_str_float(&table, &k4));
    // ht_display_str_float(&table);
    assert(ht_remove_key_str_float(&table, &k5));
    // ht_display_str_float(&table);
    assert(ht_remove_key_str_float(&table, &k6));
    // ht_display_str_float(&table);

    ht_delete_str_float(&table);
    string32_free(&k1, api);
    string32_free(&k2, api);
    string32_free(&k3, api);
    string32_free(&k4, api);
    string32_free(&k5, api);
    string32_free(&k6, api);
    string32_free(&k7, api);
    string32_free(&k8, api);

    printf("Hashtable Simple Test Passed!\n");
}

static void
test_hashtable_str_str(Freelist *fl)
{
    const alloc_api *api = &fl->api;
    size_t used1 = fl->used;

    string32 name_label = string32_create("Name", api);
    string32 age_label = string32_create("Age", api);
    string32 school_label = string32_create("School", api);
    string32 occupation_label = string32_create("Occupation", api);
    string32 name = string32_create("Mohnish Sharma", api);
    string32 age = string32_create("32 Years", api);
    string32 school = string32_create("Vishwa Bharati Public School", api);
    string32 occupation = string32_create("Game Engine Developer", api);
    string32 ayu_name = string32_create("Ayushi Sharma", api);
    string32 ayu_age = string32_create("19 Years", api);
    string32 ayu_school = string32_create("Bal Bharati Public School", api);
    string32 ayu_occupation = string32_create("An Actual Genius. I don't need no school", api);

    size_t used2 = fl->used;

    htable_str_str table;
    ht_init_str_str(&table, 0, .2f, .7f, 31, api);
    ht_add_str_str(&table, &name_label, &name);
    ht_add_str_str(&table, &age_label, &age);
    ht_add_str_str(&table, &school_label, &school);
    ht_add_str_str(&table, &occupation_label, &occupation);
    ht_display_str_str(&table);

    ht_remove_key_str_str(&table, &school_label);
    ht_display_str_str(&table);

    size_t used3 = fl->used;

    ht_add_str_str(&table, &name_label, &ayu_name);
    ht_add_str_str(&table, &age_label, &ayu_age);
    ht_add_str_str(&table, &school_label, &ayu_school);
    ht_add_str_str(&table, &occupation_label, &ayu_occupation);
    ht_display_str_str(&table);

    ht_delete_str_str(&table);
    assert(fl->used == used2);

    string32_free(&name_label, api);
    string32_free(&age_label, api);
    string32_free(&school_label, api);
    string32_free(&occupation_label, api);
    string32_free(&name, api);
    string32_free(&age, api);
    string32_free(&school, api);
    string32_free(&occupation, api);
    string32_free(&ayu_name, api);
    string32_free(&ayu_age, api);
    string32_free(&ayu_school, api);
    string32_free(&ayu_occupation, api);
    assert(fl->used == used1);

    printf("Simple Tests for string string table passed!\n");
}

static void
test_hashtable_initialization(const alloc_api *api)
{
    printf("Testing Hashtable Initialization...\n");

    // Test minimal capacity
    {
        htable_str_float table;
        ht_init_str_float(&table, 1, 0.1f, 0.5f, 0, api);
        TEST_ASSERT(table.capacity >= 1, "Minimal capacity initialization");
        ht_delete_str_float(&table);
    }

    // Test extreme load factors
    {
        htable_str_float table1, table2;
        ht_init_str_float(&table1, 16, 0.005f, 0.01f, 31, api);
        ht_init_str_float(&table2, 16, 0.1f, 0.99f, 31, api);

        TEST_ASSERT(table1.max_load_factor == 0.01f, "Minimal load factor");
        TEST_ASSERT(table2.max_load_factor == 0.99f, "Maximal load factor");

        ht_delete_str_float(&table1);
        ht_delete_str_float(&table2);
    }

    // Test seed variations
    {
        htable_str_float table1, table2;
        ht_init_str_float(&table1, 16, 0.1f, 0.5f, 0, api);
        ht_init_str_float(&table2, 16, 0.1f, 0.5f, UINT_MAX, api);

        TEST_ASSERT(table1.seed == 0, "Zero seed initialization");
        TEST_ASSERT(table2.seed == UINT_MAX, "Maximum seed initialization");

        ht_delete_str_float(&table1);
        ht_delete_str_float(&table2);
    }
}

static void
test_hashtable_basic_operations(const alloc_api *api)
{
    printf("Testing Basic Hash Table Operations...\n");

    // Create test keys
    string32 keys[10];
    const char *key_strings[] = {"Hello", "World",          "Test",   "Key",      "Hash",
                                 "Table", "Implementation", "Robust", "Thorough", "Comprehensive"};

    for (int i = 0; i < 10; i++)
    {
        keys[i] = string32_create(key_strings[i], api);
    }

    // Create hashtable
    htable_str_float table;
    ht_init_str_float(&table, 16, 0.1f, 0.75f, 31, api);

    // Test adding and retrieving values
    for (int i = 0; i < 10; i++)
    {
        float test_value = (float)(i * 3.14159);
        ht_add_str_float(&table, &keys[i], test_value);

        float retrieved_value;
        TEST_ASSERT(ht_get_str_float(&table, &keys[i], &retrieved_value), "Retrieve added key");
        TEST_ASSERT(fabs(retrieved_value - test_value) < 1e-6, "Retrieved value matches added value");
    }

    // Test key existence
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT(ht_key_exists_str_float(&table, &keys[i]), "Key exists after addition");
    }

    // Test overwriting existing keys
    for (int i = 0; i < 10; i++)
    {
        float new_value = (float)(i * 2.71828);
        ht_add_str_float(&table, &keys[i], new_value);

        float retrieved_value;
        TEST_ASSERT(ht_get_str_float(&table, &keys[i], &retrieved_value), "Retrieve overwritten key");
        TEST_ASSERT(fabs(retrieved_value - new_value) < 1e-6, "Value correctly overwritten");
        assert(table.count == 10);
    }

    // Cleanup
    ht_delete_str_float(&table);
    for (int i = 0; i < 10; i++)
    {
        string32_free(&keys[i], api);
    }
}

static void
test_hashtable_edge_cases(const alloc_api *api)
{
    printf("Testing Hash Table Edge Cases...\n");

    // Extreme value insertions
    {
        htable_str_float table;
        ht_init_str_float(&table, 16, 0.1f, 0.5f, 31, api);

        // Create keys for extreme values
        string32 key_min = string32_create("MinFloat", api);
        string32 key_max = string32_create("MaxFloat", api);
        string32 key_nan = string32_create("NaNFloat", api);
        string32 key_inf = string32_create("InfFloat", api);

        // Insert extreme values
        ht_add_str_float(&table, &key_min, FLT_MIN);
        ht_add_str_float(&table, &key_max, FLT_MAX);
        ht_add_str_float(&table, &key_nan, NAN);
        ht_add_str_float(&table, &key_inf, INFINITY);

        // Retrieve and verify
        float retrieved_value;
        TEST_ASSERT(ht_get_str_float(&table, &key_min, &retrieved_value), "Retrieve MIN float");
        TEST_ASSERT(retrieved_value == FLT_MIN, "Verify MIN float value");

        TEST_ASSERT(ht_get_str_float(&table, &key_max, &retrieved_value), "Retrieve MAX float");
        TEST_ASSERT(retrieved_value == FLT_MAX, "Verify MAX float value");

        TEST_ASSERT(ht_get_str_float(&table, &key_nan, &retrieved_value), "Retrieve NaN float");
        TEST_ASSERT(isnan(retrieved_value), "Verify NaN float value");

        TEST_ASSERT(ht_get_str_float(&table, &key_inf, &retrieved_value), "Retrieve Infinity float");
        TEST_ASSERT(isinf(retrieved_value), "Verify Infinity float value");

        // Cleanup
        ht_delete_str_float(&table);
        string32_free(&key_min, api);
        string32_free(&key_max, api);
        string32_free(&key_nan, api);
        string32_free(&key_inf, api);
    }

    // Very long key test
    {
        htable_str_float table;
        ht_init_str_float(&table, 16, 0.1f, 0.5f, 31, api);

        // Create a very long key
        char long_key_str[1024];
        memset(long_key_str, 'A', sizeof(long_key_str) - 1);
        long_key_str[sizeof(long_key_str) - 1] = '\0';

        string32 long_key = string32_create(long_key_str, api);
        ht_add_str_float(&table, &long_key, 42.0f);

        float retrieved_value;
        TEST_ASSERT(ht_get_str_float(&table, &long_key, &retrieved_value), "Retrieve long key");
        TEST_ASSERT(retrieved_value == 42.0f, "Verify long key value");

        ht_delete_str_float(&table);
        string32_free(&long_key, api);
    }

    // Null value retrieval
    {
        htable_str_float table;
        ht_init_str_float(&table, 16, 0.1f, 0.5f, 31, api);

        string32 key = string32_create("TestKey", api);
        ht_add_str_float(&table, &key, 3.14f);

        // Test retrieval with NULL value pointer
        TEST_ASSERT(ht_get_str_float(&table, &key, NULL) == true, "Retrieve with NULL value pointer");

        // Nonexistent key with NULL value pointer
        string32 non_key = string32_create("NonExistentKey", api);
        TEST_ASSERT(ht_get_str_float(&table, &non_key, NULL) == false,
                    "Retrieve nonexistent key with NULL value pointer");

        ht_delete_str_float(&table);
        string32_free(&key, api);
        string32_free(&non_key, api);
    }
}

static void
test_hashtable_resize_and_clear(const alloc_api *api)
{
    printf("Testing Hash Table Resize and Clear...\n");

    // Manual resize test
    {
        htable_str_float table;
        ht_init_str_float(&table, 16, 0.1f, 0.5f, 31, api);

        // Create and add multiple keys
        string32 keys[20];
        for (int i = 0; i < 20; i++)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "ResizeKey%d", i);
            keys[i] = string32_create(buffer, api);
            ht_add_str_float(&table, &keys[i], (float)i);
        }

        // Remember original capacity
        size_t original_capacity = table.capacity;

        // Manual resize
        ht_resize_str_float(&table, original_capacity * 2);
        TEST_ASSERT(table.capacity == original_capacity * 2, "Manual resize");

        // Verify all keys still exist after resize
        for (int i = 0; i < 20; i++)
        {
            float retrieved_value;
            TEST_ASSERT(ht_get_str_float(&table, &keys[i], &retrieved_value), "Key exists after resize");
            TEST_ASSERT(retrieved_value == (float)i, "Value preserved after resize");
        }

        // Cleanup
        ht_delete_str_float(&table);
        for (int i = 0; i < 20; i++)
        {
            string32_free(&keys[i], api);
        }
    }

    // Clear and reuse test
    {
        htable_str_float table;
        ht_init_str_float(&table, 16, 0.1f, 0.5f, 31, api);

        // Create and add multiple keys
        string32 keys[10];
        for (int i = 0; i < 10; i++)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "ClearKey%d", i);
            keys[i] = string32_create(buffer, api);
            ht_add_str_float(&table, &keys[i], (float)i);
        }

        // Clear the table
        ht_clear_str_float(&table);

        // Verify all keys are removed
        for (int i = 0; i < 10; i++)
        {
            float retrieved_value;
            TEST_ASSERT(ht_get_str_float(&table, &keys[i], &retrieved_value) == false, "Key removed after clear");
        }

        // Reuse the table
        for (int i = 0; i < 10; i++)
        {
            ht_add_str_float(&table, &keys[i], (float)(i * 2));
        }

        // Verify new values
        for (int i = 0; i < 10; i++)
        {
            float retrieved_value;
            TEST_ASSERT(ht_get_str_float(&table, &keys[i], &retrieved_value), "Key added after clear");
            TEST_ASSERT(retrieved_value == (float)(i * 2), "New value set after clear");
        }

        // Cleanup
        ht_delete_str_float(&table);
        for (int i = 0; i < 10; i++)
        {
            string32_free(&keys[i], api);
        }
    }
}

static void
test_hashtable_resize_and_removals(alloc_api *api)
{
    string32 keys[20];
    for (int i = 0; i < 20; i++)
    {
        char key_buffer[32];
        snprintf(key_buffer, sizeof(key_buffer), "Key_%d", i);
        keys[i] = string32_create(key_buffer, api);
    }

    htable_str_float table;
    ht_init_str_float(&table, 4, 0.2f, 0.7f, 31, api);

    printf("Test 1: Inserting keys to trigger resize\n");
    for (int i = 0; i < 15; i++)
    {
        ht_add_str_float(&table, &keys[i], (float)(i * 1.5));

        // ht_display_str_float(&table);
    }

    printf("Verifying inserted keys\n");
    for (int i = 0; i < 15; i++)
    {
        float value;
        assert(ht_get_str_float(&table, &keys[i], &value));
        assert(value == (float)(i * 1.5));
    }

    printf("Test 2: Removing keys to test compaction\n");
    for (int i = 0; i < 10; i++)
    {
        bool removed = ht_remove_key_str_float(&table, &keys[i]);
        assert(removed);

        // Optional: Print table state after each removal
        // ht_display_str_float(&table);
    }

    printf("Verifying remaining keys\n");
    for (int i = 10; i < 15; i++)
    {
        float value;
        assert(ht_get_str_float(&table, &keys[i], &value));
        assert(value == (float)(i * 1.5));
    }

    printf("Test 3: Re-inserting keys after removals\n");
    for (int i = 0; i < 10; i++)
    {
        ht_add_str_float(&table, &keys[i], (float)(i * 2.0));
    }

    printf("Verifying re-inserted keys\n");
    for (int i = 0; i < 15; i++)
    {
        float value;
        assert(ht_get_str_float(&table, &keys[i], &value));
    }

    printf("Test 4: Stress test with multiple operations\n");
    for (int cycle = 0; cycle < 5; cycle++)
    {
        for (int i = 0; i < 20; i++)
        {
            ht_add_str_float(&table, &keys[i], (float)(i * 3.0 + cycle));
        }

        for (int i = 0; i < 20; i += 2)
        {
            ht_remove_key_str_float(&table, &keys[i]);
        }
    }

    printf("Test 5: Verifying key states after stress test\n");
    for (int i = 0; i < 20; i++)
    {
        float value;
        if (i % 2 == 1)
        {
            assert(ht_get_str_float(&table, &keys[i], &value));
            assert(value == (float)(i * 3.0 + 4)); // Last cycle's value
        }
        else
        {
            assert(!ht_get_str_float(&table, &keys[i], NULL));
        }
    }

    ht_delete_str_float(&table);
    for (int i = 0; i < 20; i++)
    {
        string32_free(&keys[i], api);
    }

    printf("Hashtable Resize and Operations Test PASSED!\n");
}

static void
test_hashtable_stress_test(const alloc_api *api)
{
    printf("Running Hash Table Stress Test...\n");

    htable_str_float table;
    ht_init_str_float(&table, 16, 0.1f, 0.75f, 31, api);

    // Large number of insertions to test resizing and collision handling
    const int STRESS_TEST_SIZE = 10000;
    string32 *keys = malloc(STRESS_TEST_SIZE * sizeof(string32));

    // Create unique keys
    for (int i = 0; i < STRESS_TEST_SIZE; i++)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "StressKey_%d_UniqueString", i);
        keys[i] = string32_create(buffer, api);
        ht_add_str_float(&table, &keys[i], (float)i);
    }
    // ht_display_str_float(&table);

    // Verify all keys
    for (int i = 0; i < STRESS_TEST_SIZE; i++)
    {
        float retrieved_value;
        TEST_ASSERT(ht_get_str_float(&table, &keys[i], &retrieved_value), "Stress test key retrieval");
        TEST_ASSERT(retrieved_value == (float)i, "Stress test value verification");
    }

    // Cleanup
    ht_delete_str_float(&table);
    for (int i = 0; i < STRESS_TEST_SIZE; i++)
    {
        string32_free(&keys[i], api);
    }
    free(keys);
}

#ifndef FREELIST_ALLOCATOR_IMPLEMENTATION
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#endif
#include <memory/freelist_alloc.h>

void htable_unit_tests()
{
    freelist_create(fl, MEGABYTES(256), 0, PLACEMENT_POLICY_FIND_BEST);
    alloc_api *api = freelist_get_api(&fl);
    string32_set_global_allocator(api);

    test_hashtable_simple(&fl);
    assert(fl.used == 0);
    test_hashtable_resize_and_removals(api);
    assert(fl.used == 0);
    test_hashtable_str_str(&fl);
    assert(fl.used == 0);
    test_hashtable_initialization(api);
    assert(fl.used == 0);
    test_hashtable_basic_operations(api);
    assert(fl.used == 0);
    test_hashtable_edge_cases(api);
    assert(fl.used == 0);
    test_hashtable_resize_and_clear(api);
    assert(fl.used == 0);
    test_hashtable_stress_test(api);
    assert(fl.used == 0);

    printf("ALL HASH TABLE TESTS PASSED SUCCESSFULLY!\n");

    free(fl.data);
    int x = 0;
}

#endif
#endif
#endif
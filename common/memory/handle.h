#ifndef HANDLE_H
#define HANDLE_H

#include "memory/memory.h"

#include <utility>
#include <memory/freelist2_alloc.h>
#include <typeinfo>
#include <new>

#ifdef ALLOCATOR_DEBUG
// #include <concepts>
#include <type_traits>
#ifndef STRING32_IMPLEMENTATION
#define STRING32_IMPLEMENTATION
#endif
#include <containers/string_utils.h>
#include <string.h>
#endif

#define Stringify(t) #t

template <typename T>
struct Handle
{
public:
    T* owner;
    const alloc_api *allocApi;

#ifdef ALLOCATOR_DEBUG
    string32 labelString;
#endif

    template <typename... Args>
    using has_normal_constructor = std::is_constructible<T, Args...>;
    template <typename... Args>
    using has_alloc_constructor = std::is_constructible<T, const alloc_api *, Args...>;

  public:
    Handle() : owner(nullptr), allocApi(nullptr) {}

    /* The handled object should have constructor for passed in Args(one which does not have alloc_api as the first
    argument) or should have constructor which accepts alloc_api as the first arg and then the Args passed in
    here. */
    template <typename... Args, typename = typename std::enable_if<has_normal_constructor<Args...>::value ||
                                                                   has_alloc_constructor<Args...>::value>::type>
#ifdef ALLOCATOR_DEBUG
    explicit Handle(const alloc_api *api, const char *label, Args &&...args) noexcept
        : allocApi(api)
    {
        assert(api != nullptr && "alloc api should be passed to the handle to manage memory");
        owner = static_cast<T *>(api->alloc(api->allocator, sizeof(T)));
        freelist2_set_allocation_owner((Freelist2 *)api->allocator, (void *)owner, (void **)&owner);

        labelString = string32_create(label, NULL);
        freelist2_set_allocation_label((Freelist2 *)api->allocator, owner, string32_cstr(&labelString));
        construct_impl(std::forward<Args>(args)...);
    }
#else
    explicit Handle(const alloc_api *api, Args &&...args) noexcept
        : allocApi(api)
    {
        assert(api != nullptr && "alloc api should be passed to the handle to manage memory");
        owner = static_cast<T *>(api->alloc(api->allocator, sizeof(T)));
        freelist2_set_allocation_owner((Freelist2 *)api->allocator, (void *)owner, (void **)&owner);
        construct_impl(std::forward<Args>(args)...);
    }
#endif

    Handle(Handle &&other) noexcept
        : owner(other.owner), allocApi(other.allocApi)
    {
        other.owner = nullptr;
        other.allocApi = nullptr;
#ifdef ALLOCATOR_DEBUG
        labelString = other.labelString;
        memset(&other.labelString, 0, sizeof(string32));
#endif
        freelist2_set_allocation_owner((Freelist2 *)allocApi->allocator, (void *)owner, (void **)&owner);
    }

    Handle&
    operator=(Handle &&other) noexcept
    {
        if (this != &other) {
            owner = other.owner;
            allocApi = other.allocApi;
#ifdef ALLOCATOR_DEBUG
            labelString = other.labelString;
            memset((void *)other.labelString, 0, sizeof(string32));
#endif
            other.owner = nullptr;
            other.allocApi = nullptr;
            freelist2_set_allocation_owner((Freelist2 *)allocApi->allocator, (void *)owner, (void **)&owner);
        }
        return *this;
    }

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    T& operator*() const noexcept {
        assert(owner);
        return *owner;
    }

    template<typename U = T>
    typename std::enable_if<std::is_class<U>::value, U*>::type
    operator->() noexcept {
        assert(owner);
        return owner;
    }

    void free() noexcept {
        if(owner && allocApi) {
            /* printf("%s destructor called!\n", typeid(T).name()); */
            destroy_impl();
            allocApi->free(allocApi->allocator, owner);
            owner = nullptr;
            allocApi = nullptr;
#ifdef ALLOCATOR_DEBUG
            string32_cstr_free(&labelString, nullptr);
#endif
        }
    }

    ~Handle() { free(); }

    template <typename U, typename... Args>
#ifdef ALLOCATOR_DEBUG
    friend void EmplaceHandle(Handle<U> *where, const alloc_api *api, const char *label, Args &&...args);
#else
    friend void EmplaceHandle(Handle<U> *where, const alloc_api *api, Args &&...args);
#endif

  private:
    template <typename... Args>
    typename std::enable_if<has_alloc_constructor<Args...>::value>::type
    construct_impl(Args &&...args) {
        new (owner) T(allocApi, std::forward<Args>(args)...);
    }

    template <typename... Args>
    typename std::enable_if<!has_alloc_constructor<Args...>::value>::type
    construct_impl(Args &&...args) {
        new (owner) T(std::forward<Args>(args)...);
    }

    template <typename U = T>
    typename std::enable_if<!std::is_trivially_destructible<U>::value>::type
    destroy_impl() {
        owner->~T();
    }

    template <typename U = T>
    typename std::enable_if<std::is_trivially_destructible<U>::value>::type
    destroy_impl() {}
};

template<typename T, typename... Args>
#ifdef ALLOCATOR_DEBUG
Handle<T> MakeHandle(const alloc_api* api, const char *label, Args&&... args) {
    return Handle<T>(api, label, std::forward<Args>(args)...);
}
#else
Handle<T> MakeHandle(const alloc_api* api, Args&&... args) {
    return Handle<T>(api, std::forward<Args>(args)...);
}
#endif

// Helper function for in-place construction
template <typename T, typename... Args>
#ifdef ALLOCATOR_DEBUG
void
EmplaceHandle(Handle<T> *where, const alloc_api *api, const char *label, Args &&...args)
{
    static_assert(!std::is_const<T>::value, "Cannot emplace a const type");
    // storing the handle object in the provided memory passed in here.
    new (where) Handle<T>();
    where->allocApi = api;
    // this owner stores the pointer to heap memory where the object which is being "handled" is stored. The handle
    // is basically a wrapper for this memory.
    where->owner = static_cast<T *>(api->alloc(api->allocator, sizeof(T)));
    freelist2_set_allocation_owner((Freelist2 *)api->allocator, (void *)where->owner, (void **)&where->owner);

    where->labelString = string32_create(label, NULL);
    freelist2_set_allocation_label((Freelist2 *)api->allocator, where->owner, string32_cstr(&where->labelString));

    where->construct_impl(std::forward<Args>(args)...);
}
#else
void
EmplaceHandle(Handle<T> *where, const alloc_api *api, Args &&...args)
{
    static_assert(!std::is_const<T>::value, "Cannot emplace a const type");
    new (where) Handle<T>();
    where->allocApi = api;
    where->owner = static_cast<T *>(api->alloc(api->allocator, sizeof(T)));
    freelist2_set_allocation_owner((Freelist2 *)api->allocator, (void *)where->owner, (void **)&where->owner);
    where->construct_impl(std::forward<Args>(args)...);
}
#endif

template <typename T>
struct ImmutableArray
{
    ImmutableArray() : arrOwner(nullptr), length(0), api(nullptr) {}

    explicit ImmutableArray(const alloc_api *api, T *inputArray, int len) {
        // arrOwner points to some memory holding the actual array data.
        this->arrOwner = (T *)api->alloc(api->allocator, sizeof(T)*len);
        /* if this immutable array object is being managed, that means it can be moved by the allocator when
           defragmenting which means &arrOwner will be equal to the Handle's owner object.
           Handle.owner -> this.arrOwner -> the actual array memory. */
        freelist2_set_allocation_owner((Freelist2 *)api->allocator, (void *)arrOwner, (void **)&arrOwner);
        this->length = len;
        shumemcpy(this->arrOwner, inputArray, sizeof(T)*len);
        this->api = api;

#ifdef ALLOCATOR_DEBUG
        char buffer[64];
        snprintf(buffer, 64, "ImmutableArray<%s>", typeid(T).name());
        const char *label = getLabel(buffer/*, __FILE__, __LINE__*/);
        this->labelString = string32_create(label, NULL);
        freelist2_set_allocation_label((Freelist2 *)api->allocator, this->arrOwner,
                                       string32_cstr(&this->labelString));
#endif
    }

    ImmutableArray &operator=(const ImmutableArray &other) = delete;
    ImmutableArray(const ImmutableArray &other) = delete;
    ImmutableArray &operator=(ImmutableArray &&other) = delete;
    ImmutableArray(ImmutableArray &&other) = delete;

    T* operator->() const noexcept {
        return arrOwner;
    }

    ~ImmutableArray()
    {
        for (int i = 0; i < length; ++i) {
            T *t = this->arrOwner + i;
            t->~T();
        }
        api->free((Freelist2 *)api->allocator, this->arrOwner);
        this->arrOwner = nullptr;
        this->api = nullptr;
#ifdef ALLOCATOR_DEBUG
        string32_cstr_free(&labelString, nullptr);
#endif
    }

    T *arrOwner;
    int length;
    const alloc_api *api;
#ifdef ALLOCATOR_DEBUG
    string32 labelString;
#endif
};

struct test
{
    int a;
    unsigned char b;
    uint32_t c;
};

inline void
handle_unit_tests(void *memory, size_t memory_size)
{
    Freelist2 fl;
    freelist2_init(&fl, memory, memory_size, DEFAULT_ALIGNMENT);

    constexpr int numHandles = 24;
    Handle<int> originalHandles[numHandles];
    for (int i = 0; i < 24; ++i) {
#ifdef ALLOCATOR_DEBUG
        char buffer[128];
        snprintf(buffer, 128, "OriginalHandle[%d]", i);
        EmplaceHandle(&originalHandles[i], &fl.api, buffer, i);
#else
        EmplaceHandle(&originalHandles[i], &fl.api, i);
#endif
    }

    int arr1[8] = {10, 20, 30, 40, 50, 60, 70, 80};
#ifdef ALLOCATOR_DEBUG
    Handle<ImmutableArray<int>> arrHandle =
            MakeHandle<ImmutableArray<int>>(&fl.api, "arrHandle(ImmutableArray<int>)", arr1, 8);
#else
    Handle<ImmutableArray<int>> arrHandle = MakeHandle<ImmutableArray<int>>(&fl.api, arr1, 8);
#endif

    int aa = arrHandle->arrOwner[3];
    for (int i = 0; i < 24; i += 2) {
        originalHandles[i].free();
    }
    freelist2_defragment(&fl);
    assert(fl.block_count == 1);
    for (int i = 1; i < 24; i += 2) {
        int v = *originalHandles[i];
        printf("%d\n", v);
    }

    for (int i = 1; i < 24; i += 2) {
        originalHandles[i].free();
    }

    freelist2_defragment(&fl);

    for (int i = 0; i < 8; ++i) {
        assert(arrHandle->arrOwner[i] == (i+1)*10);
    }

    test t = test{32, 'v', 1024};
#ifdef ALLOCATOR_DEBUG
    Handle<test> testHandle = MakeHandle<test>(&fl.api, "testHandle", t);
#else
    Handle<test> testHandle = MakeHandle<test>(&fl.api, t);
#endif
    test value = *testHandle;
    assert(value.a == 32 && value.b == 'v' && value.c == 1024);
    testHandle->a = 12;
    value = *testHandle;
    assert(value.a == 12 && value.b == 'v' && value.c == 1024);

    test tests[32];
    for (int i = 0; i < 32; ++i) {
        tests[i].a = i;
        tests[i].b = 'a' + i;
        tests[i].c = 1000+i;
    }

#ifdef ALLOCATOR_DEBUG
    Handle<ImmutableArray<test>> testArrayHandle =
        MakeHandle<ImmutableArray<test>>(&fl.api, "testArrayHandle", tests, 32);
#else
    Handle<ImmutableArray<test>> testArrayHandle = MakeHandle<ImmutableArray<test>>(&fl.api, tests, 32);
#endif

    for (int i = 0; i < 32; ++i) {
        const test t = testArrayHandle->arrOwner[i];
        assert(t.a == i);
        assert(t.b == ('a' + i));
        assert(t.c == (1000 + i));
    }

    for (int i = 0; i < 8; ++i) {
        assert(arrHandle->arrOwner[i] == (i + 1) * 10);
    }

    arrHandle.free();
    testHandle.free();

    assert(fl.block_count > 1);
    freelist2_defragment(&fl);
    assert(fl.block_count == 1);

    for (int i = 0; i < 32; ++i) {
        const test t = testArrayHandle->arrOwner[i];
        assert(t.a == i);
        assert(t.b == ('a'+i));
        assert(t.c == (1000+i));
    }
    
    testArrayHandle.free();

    /*
     * NOTE: Verify the following:
     * 1. fl.used == 0 :- Not using any of the managed memory since I freed everything up until here.
     * 2. (fl.block_count == 1 && fl.head->block_size == fl.size) :- there is only one free block left which is
     *    basically the entire size the freelist is managing.
     * 3. fl.head == fl.data :- the head of the freelist (the only one) is the same as the start of the manged
     *    memory address which this freelist is managing.
     * 4. fl.table.count == 0 :- the hashtable contains the entries pointing to allocations. Since every allocation
     *    is freed upto this point, the hashtable should be empty.
     */
    assert(fl.used == 0 && fl.block_count == 1 && fl.head->block_size == fl.size &&
           (uintptr_t)fl.head == (uintptr_t)fl.data && fl.table.count == 0);

    freelist2_destroy(&fl);
}

#endif
#include "set.h"
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#if !defined MANY || MANY < 1
#define MANY 10
#endif

typedef struct Class {
    size_t size;
    void *(*ctor)(void *self, va_list *app);
    void *(*dtor)(void *self);
    void *(*clone)(const void *self);
    int   (*differ)(const void *self, const void *b);
} Class;

typedef struct String {
    const void *class;
    char *text;
} String;

typedef struct Set
{
    const void *class;
    unsigned int count;
} Set;

struct Object { unsigned int count; struct Set *in; };

static const size_t _Set = sizeof(struct Set);
static const size_t _Object = sizeof(struct Object);

const void *__Set = &_Set;
const void *Object = &_Object;

static int heap[MANY];


void *
new(const void *_class, ...)
{
    const struct Class *class = _class;
    void *p = calloc(1, class->size);
    assert(p);
    *(const struct Class **)p = class;
    if (class->ctor) {
        va_list ap;
        va_start(ap, _class);
        p = class->ctor(p, &ap);
        va_end(ap);
    }
    return p;
}

void
delete(void *_item)
{
    free(_item);
}

void *
add(void *_set, const void *_element)
{
    struct Set *set = _set;
    struct Object *element = (void *)_element;
    assert(set);
    assert(element);
    if (!element->in) {
        element->in = set;
    } else {
        assert(element->in == set);
    }
    ++element->count;
    ++set->count;
    return element;
}

void *
find(const void *_set, const void *_element)
{
    const struct Object *element = _element;
    assert(_set);
    assert(element);
    return element->in == _set ? (void *)element : 0;
}

int
contains(const void *_set, const void *_element)
{
    return find(_set, _element) != 0;
}

void *
drop(void *_set, const void *_element)
{
    struct Set *set = _set;
    struct Object *element = find(set, _element);
    if (element) {
        if (--element->count == 0)
            element->in = 0;
        --set->count;
    }
    return element;
}

int
differ(const void *a, const void *b)
{
    return a != b;
}

unsigned
count(const void *_set)
{
    const struct Set *set = _set;
    assert(set);
    return set->count;
}
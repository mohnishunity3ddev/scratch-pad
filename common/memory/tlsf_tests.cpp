#include "tlsf.h"
#define TLSF_INCLUDE_TESTS

#ifdef TLSF_INCLUDE_TESTS

namespace Tlsf
{

void tests()
{
    Allocator allocator;
    Allocation a1 = allocator.allocate(512);
    Allocation a2 = allocator.allocate(512);
    Allocation a3 = allocator.allocate(512);
    Allocation a4 = allocator.allocate(512);
    Allocation a5 = allocator.allocate(512);
    Allocation a6 = allocator.allocate(512);
    Allocation a7 = allocator.allocate(512);
    Allocation a8 = allocator.allocate(512);
    Allocation a9 = allocator.allocate(512);
    Allocation a10 = allocator.allocate(512);

    allocator.free(a1);
    allocator.free(a3);
    allocator.free(a5);
    allocator.free(a7);
    allocator.free(a9);


    Allocation t1 = allocator.allocate(104);
    Allocation t2 = allocator.allocate(109);
    Allocation t3 = allocator.allocate(36);
    Allocation t4 = allocator.allocate(38);
    allocator.free(a2);
    allocator.free(a4);
    allocator.free(a6);
    allocator.free(a8);
    allocator.free(a10);


    int x = 0;
}

}

#endif



#ifndef CLOCK_H
#define CLOCK_H

#include "common.h"
#include "memory/memory.h"
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#endif

struct Clock
{
    char *name;
#ifdef WIN32
    LARGE_INTEGER startTime, endTime;
#else
    uint64_t startTime, endTime;
#endif
    explicit Clock() noexcept;
    void begin(const char *name);
    void end();
    ~Clock();
};

#ifdef WIN32
static LARGE_INTEGER frequency = {};
#endif
static bool isInitialized = false;

inline Clock::Clock() noexcept
{
#ifdef WIN32
    QueryPerformanceFrequency(&frequency);
#endif
    name = NULL;
    startTime = {};
    endTime = {};
}

inline void
Clock::begin(const char *blockName)
{
#ifdef WIN32
    QueryPerformanceCounter(&startTime);
#endif
    const size_t size = strlen(blockName)+1;
    name = (char *)malloc(size);
    shumemcpy(name, blockName, size);
    isInitialized = true;
}

inline void
Clock::end()
{
#ifdef WIN32
    QueryPerformanceCounter(&endTime);
    double elapsedTime = ((double)(endTime.QuadPart - startTime.QuadPart) * 1000.0) / (double)frequency.QuadPart;
#else
    // TODO: implement this for other platforms.
    double elapsedTime = endTime-startTime;
#endif
    printf("[%s]: %.3f miliseconds \n", name, elapsedTime);
}

inline Clock::~Clock()
{
    if (name) {
        free(name);
    }
}

#endif
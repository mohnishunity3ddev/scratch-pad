#ifndef CLOCK_H
#define CLOCK_H

#include "common.h"
#include "memory/memory.h"
#include <stdio.h>
#include <windows.h>

struct Clock
{
    char *name;
    LARGE_INTEGER startTime, endTime;
    explicit Clock() noexcept;
    void begin(const char *name);
    void end();
    ~Clock();
};

static LARGE_INTEGER frequency = {};
static bool isInitialized = false;

inline Clock::Clock() noexcept
{
    QueryPerformanceFrequency(&frequency);
    name = NULL;
    startTime = {};
    endTime = {};
}

inline void
Clock::begin(const char *blockName)
{
    QueryPerformanceCounter(&startTime);
    const size_t size = strlen(blockName)+1;
    name = (char *)malloc(size);
    shumemcpy(name, blockName, size);
    isInitialized = true;
}

inline void
Clock::end()
{
    QueryPerformanceCounter(&endTime);
    double elapsedTime = ((double)(endTime.QuadPart - startTime.QuadPart) * 1000.0) / (double)frequency.QuadPart;
    printf("[%s]: %.3f miliseconds \n", name, elapsedTime);
}

inline Clock::~Clock()
{
    if (name) {
        free(name);
    }
}

#endif
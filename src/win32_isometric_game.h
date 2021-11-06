#ifndef WIN32_ISOMETRIC_GAME_H
#define WIN32_ISOMETRIC_GAME_H
#include "utils.h"
#include <windows.h>

struct Win32_Offscreen_Buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct Win32_Window_Dimensions
{
    int width;
    int height;
};

struct Win32_Sound_Output
{
    int samplesPerSecond;
    int bytesPerSample;
    u32 runningsampleIndex;
    int secondaryBufferSize;
    int latencySampleCount;
};

#endif

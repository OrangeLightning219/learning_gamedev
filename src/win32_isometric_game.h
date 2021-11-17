#ifndef WIN32_ISOMETRIC_GAME_H
#define WIN32_ISOMETRIC_GAME_H

#ifndef UNITY_BUILD
#endif

#include "isometric_game.h"
#include <windows.h>

struct Win32_Offscreen_Buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
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
    DWORD secondaryBufferSize;
    int safetyBytes;
};

struct Win32_Debug_Sound_Marker
{
    DWORD outputPlayCursor;
    DWORD outputWriteCursor;
    DWORD outputLocation;
    DWORD outputByteCount;
    DWORD expectedFlipPlayCursor;
    DWORD flipPlayCursor;
    DWORD flipWriteCursor;
};

struct Win32_Game_Code
{
    HMODULE gameCodeDLL;
    FILETIME dllLastWriteTime;
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;
    bool isValid;
};

struct Win32_Replay_Buffer
{
    HANDLE memoryMap;
    HANDLE fileHandle;
    char filename[ MAX_PATH ];
    void *memoryBlock;
};

struct Win32_State
{
    char exePath[ MAX_PATH ];
    char *exeFilename;

    u64 totalSize;
    void *gameMemoryBlock;
    Win32_Replay_Buffer replayBuffers[ 4 ];

    HANDLE recordingHandle;
    int inputRecordingIndex;

    HANDLE playbackHandle;
    int inputPlayingIndex;
};
#endif

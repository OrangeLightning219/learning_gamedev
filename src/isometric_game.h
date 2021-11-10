#ifndef ISOMETRIC_GAME_H
#define ISOMETRIC_GAME_H
#include "utils.h"

struct Game_Sound_Output_Buffer
{
    int samplesPerSecond;
    int sampleCount;
    s16 *samples;
};

struct Game_Offscreen_Buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};

struct Game_Button_State
{
    int halfTransitionCount;
    bool endedDown;
};

struct Game_Controller_Input
{
    bool isAnalog;
    bool isConnected;

    float32 leftStickAverageX, leftStickAverageY;
    float32 rightStickAverageX, rightStickAverageY;
    float32 leftTriggerAverage;
    float32 rightTriggerAverage;

    union
    {
        Game_Button_State buttons[ 14 ];
        struct
        {
            Game_Button_State up;
            Game_Button_State down;
            Game_Button_State left;
            Game_Button_State right;
            Game_Button_State a;
            Game_Button_State b;
            Game_Button_State x;
            Game_Button_State y;
            Game_Button_State l1;
            Game_Button_State r1;
            Game_Button_State l3;
            Game_Button_State r3;
            Game_Button_State start;
            Game_Button_State back;
        };
    };
};

struct Game_Input
{
    Game_Controller_Input controllers[ 5 ];
};

inline Game_Controller_Input *getController( Game_Input *input, int controllerIndex )
{
    Assert( controllerIndex < ( int ) ArrayCount( input->controllers ) );
    return &input->controllers[ controllerIndex ];
}

struct Game_Memory
{
    bool isInitialized;
    u64 permanentStorageSize;
    void *permanentStorage;
    u64 transientStorageSize;
    void *transientStorage;
};

// services for the platform layer
internal void GameUpdateAndRender( Game_Memory *memory, Game_Input *input,
                                   Game_Offscreen_Buffer *buffer, Game_Sound_Output_Buffer *soundBuffer );

// services from the platform layer
#if INTERNAL
struct Debug_Read_File_Result
{
    u32 contentSize;
    void *content;
};
internal Debug_Read_File_Result DebugPlatformReadEntireFile( char *filename );
internal void DebugPlatformFreeFileMemory( void *fileMemory );

internal bool DebugPlatformWriteEntireFile( char *filename, u32 memorySize, void *memory );
#endif

// -------------------------------------
struct Game_State
{
    int xOffset;
    int yOffset;
    int toneHz;
};

#endif

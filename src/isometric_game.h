#ifndef ISOMETRIC_GAME_H
#define ISOMETRIC_GAME_H
#ifndef UNITY_BUILD
#endif

#include <math.h>
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
    int bytesPerPixel;
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

// services from the platform layer
#ifdef INTERNAL
struct Debug_Read_File_Result
{
    u32 contentSize;
    void *content;
};

    #define DEBUG_PLATFORM_READ_ENTIRE_FILE( name ) Debug_Read_File_Result name( char *filename )
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE( debug_platform_read_entire_file );

    #define DEBUG_PLATFORM_FREE_FILE_MEMORY( name ) void name( void *fileMemory )
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY( debug_platform_free_file_memory );

    #define DEBUG_PLATFORM_WRITE_ENTIRE_FILE( name ) bool name( char *filename, u32 memorySize, void *memory )
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE( debug_platform_write_entire_file );

#endif

struct Game_Memory
{
    bool isInitialized;
    u64 permanentStorageSize;
    void *permanentStorage;
    u64 transientStorageSize;
    void *transientStorage;
#ifdef INTERNAL
    debug_platform_read_entire_file *DebugPlatformReadEntireFile;
    debug_platform_write_entire_file *DebugPlatformWriteEntireFile;
    debug_platform_free_file_memory *DebugPlatformFreeFileMemory;
#endif
};

// services for the platform layer
#define GAME_UPDATE_AND_RENDER( name ) void name( Game_Memory *memory, Game_Input *input, Game_Offscreen_Buffer *buffer )
typedef GAME_UPDATE_AND_RENDER( game_update_and_render );
GAME_UPDATE_AND_RENDER( GameUpdateAndRenderStub ) { } //NOLINT

#define GAME_GET_SOUND_SAMPLES( name ) void name( Game_Memory *memory, Game_Sound_Output_Buffer *soundBuffer )
typedef GAME_GET_SOUND_SAMPLES( game_get_sound_samples );
GAME_GET_SOUND_SAMPLES( GameGetSoundSamplesStub ) { } //NOLINT

// -------------------------------------
struct Game_State
{
    int xOffset;
    int yOffset;
    int toneHz;
    float32 tSine;

    int playerX;
    int playerY;
};

#endif

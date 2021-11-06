#ifndef ISOMETRIC_GAME_H
#define ISOMETRIC_GAME_H

#include "utils.h"

#define ArrayCount( array ) ( sizeof( array ) / sizeof( ( array )[ 0 ] ) )

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

    float32 minStickX, maxStickX;
    float32 minStickY, maxStickY;
    float32 leftStickStartX, rightStickStartX;
    float32 leftStickStartY, rightStickStartY;
    float32 leftStickEndX, rightStickEndX;
    float32 leftStickEndY, rightStickEndY;

    float32 minTrigger, maxTrigger;
    float32 leftTriggerStart, leftTriggerEnd;
    float32 rightTriggerStart, rightTriggerEnd;

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
    Game_Controller_Input controllers[ 4 ];
};
// services for the platform layer
internal void GameUpdateAndRender( Game_Input *input, Game_Offscreen_Buffer *buffer, Game_Sound_Output_Buffer *soundBuffer );

// services from the platform layer

#endif

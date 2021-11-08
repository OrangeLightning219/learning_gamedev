#include "isometric_game.h"
#include <math.h>

internal void GameOutputSound( Game_Sound_Output_Buffer *buffer, int toneHz )
{
    local_persist float32 tSine = 0;
    s16 toneVolume = 1000;
    int wavePeriod = buffer->samplesPerSecond / toneHz;

    s16 *sampleOut = buffer->samples;
    for ( int sampleIndex = 0; sampleIndex < buffer->sampleCount; ++sampleIndex )
    {
        tSine += 2 * pi32 / ( float32 ) wavePeriod;
        float32 sineValue = sinf( tSine );
        s16 sampleValue = ( s16 ) ( sineValue * toneVolume );
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
    }
}
internal void RenderWeirdGradient( Game_Offscreen_Buffer *buffer, int xOffset, int yOffset )
{
    u8 *row = ( u8 * ) buffer->memory;
    for ( int y = 0; y < buffer->height; ++y )
    {
        u32 *pixel = ( u32 * ) row;
        for ( int x = 0; x < buffer->width; ++x )
        {
            u8 red = x + y + xOffset + yOffset;
            u8 green = y + yOffset;
            u8 blue = x + xOffset;
            *pixel++ = 0 | red << 16 | green << 8 | blue;
        }
        row += buffer->pitch;
    }
}

internal void GameUpdateAndRender( Game_Memory *memory, Game_Input *input,
                                   Game_Offscreen_Buffer *buffer, Game_Sound_Output_Buffer *soundBuffer )
{
    Assert( sizeof( Game_State ) <= memory->permanentStorageSize );
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;

    if ( !memory->isInitialized )
    {
        gameState->toneHz = 256;

        char *filename = __FILE__;
        Debug_Read_File_Result file = DebugPlatformReadEntireFile( filename );
        if ( file.content )
        {
            DebugPlatformWriteEntireFile( "test.out", file.contentSize, file.content );
            DebugPlatformFreeFileMemory( file.content );
        }

        memory->isInitialized = true;
    }

    Game_Controller_Input *input0 = &input->controllers[ 0 ];
    if ( input0->isAnalog )
    {
        gameState->toneHz = 256 + ( int ) ( 128.0f * input0->leftStickEndY );
        gameState->xOffset -= ( int ) ( 4.0f * input0->leftStickEndX );
    }
    else
    {
    }

    if ( input0->a.endedDown )
    {
        gameState->yOffset += 1;
    }

    GameOutputSound( soundBuffer, gameState->toneHz );
    RenderWeirdGradient( buffer, gameState->xOffset, gameState->yOffset );
}

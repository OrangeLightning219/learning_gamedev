#include "isometric_game.h"
#ifndef UNITY_BUILD
#endif

void GameOutputSound( Game_Sound_Output_Buffer *buffer, Game_State *gameState )
{
    s16 toneVolume = 1000;
    int wavePeriod = buffer->samplesPerSecond / gameState->toneHz;

    s16 *sampleOut = buffer->samples;
    for ( int sampleIndex = 0; sampleIndex < buffer->sampleCount; ++sampleIndex )
    {
        gameState->tSine += 2 * pi32 / ( float32 ) wavePeriod;
        if ( gameState->tSine > 2 * pi32 )
        {
            gameState->tSine -= 2 * pi32;
        }
        float32 sineValue = sinf( gameState->tSine );
        s16 sampleValue = ( s16 ) ( sineValue * toneVolume );
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
    }
}
void RenderWeirdGradient( Game_Offscreen_Buffer *buffer, int xOffset, int yOffset )
{
    u8 *row = ( u8 * ) buffer->memory;
    for ( int y = 0; y < buffer->height; ++y )
    {
        u32 *pixel = ( u32 * ) row;
        for ( int x = 0; x < buffer->width; ++x )
        {
            u8 red = ( u8 ) ( x + y + xOffset + yOffset );
            u8 green = ( u8 ) ( y + yOffset );
            u8 blue = ( u8 ) ( y + xOffset );
            *pixel++ = 0 | red << 16 | green << 8 | blue;
        }
        row += buffer->pitch;
    }
}

extern "C" __declspec( dllexport )
GAME_UPDATE_AND_RENDER( GameUpdateAndRender )
{
    Assert( sizeof( Game_State ) <= memory->permanentStorageSize );
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;

    if ( !memory->isInitialized )
    {
        gameState->toneHz = 256;
        gameState->tSine = 0.0f;

        char *filename = __FILE__;
        Debug_Read_File_Result file = memory->DebugPlatformReadEntireFile( filename );
        if ( file.content )
        {
            memory->DebugPlatformWriteEntireFile( "test.out", file.contentSize, file.content );
            memory->DebugPlatformFreeFileMemory( file.content );
        }

        memory->isInitialized = true;
    }
    for ( int controllerIndex = 0; controllerIndex < ArrayCount( input->controllers ); ++controllerIndex )
    {
        Game_Controller_Input *controller = getController( input, controllerIndex );
        if ( controller->isAnalog )
        {
            gameState->toneHz = 256 + ( int ) ( 128.0f * controller->leftStickAverageY );
            gameState->xOffset -= ( int ) ( 4.0f * controller->leftStickAverageX );
        }
        else
        {
            if ( controller->left.endedDown )
            {
                gameState->xOffset += 1;
            }
            else if ( controller->right.endedDown )
            {
                gameState->xOffset -= 1;
            }
        }

        if ( controller->a.endedDown )
        {
            gameState->yOffset += 1;
        }
    }
    RenderWeirdGradient( buffer, gameState->xOffset, gameState->yOffset );
}

extern "C" __declspec( dllexport )
GAME_GET_SOUND_SAMPLES( GameGetSoundSamples )
{
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;
    GameOutputSound( soundBuffer, gameState );
}

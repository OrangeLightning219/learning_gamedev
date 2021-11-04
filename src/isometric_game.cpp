#include "isometric_game.h"

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
            *pixel++ = 0x00000000 | red << 16 | green << 8 | blue;
        }
        row += buffer->pitch;
    }
}

internal void GameUpdateAndRender( Game_Offscreen_Buffer *buffer, int xOffset, int yOffset )
{
    RenderWeirdGradient( buffer, xOffset, yOffset );
}

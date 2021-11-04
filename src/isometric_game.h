#ifndef ISOMETRIC_GAME_H
#define ISOMETRIC_GAME_H

struct Game_Offscreen_Buffer
{
    void *memory;
    int width;
    int height;
    int pitch;
};
// services for the platform layer
internal void GameUpdateAndRender( Game_Offscreen_Buffer *buffer, int xOffset, int yOffset );

// services from the platform layer

#endif

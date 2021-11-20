#include "isometric_game.h"
#ifndef UNITY_BUILD
#endif

internal void GameOutputSound( Game_Sound_Output_Buffer *buffer, Game_State *gameState )
{
    s16 *sampleOut = buffer->samples;
    for ( int sampleIndex = 0; sampleIndex < buffer->sampleCount; ++sampleIndex )
    {
        s16 sampleValue = 0;
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;
    }
}

internal void DrawRectangle( Game_Offscreen_Buffer *buffer, float32 floatMinX, float32 floatMinY, float32 floatMaxX, float32 floatMaxY,
                             float32 r, float32 g, float32 b )
{
    s32 minX = RoundFloat32ToS32( floatMinX );
    s32 minY = RoundFloat32ToS32( floatMinY );
    s32 maxX = RoundFloat32ToS32( floatMaxX );
    s32 maxY = RoundFloat32ToS32( floatMaxY );

    if ( minX < 0 ) { minX = 0; }
    if ( minY < 0 ) { minY = 0; }
    if ( maxX > buffer->width ) { maxX = buffer->width; }
    if ( maxY > buffer->height ) { maxY = buffer->height; }

    u32 color = ( RoundFloat32ToU32( r * 255.0f ) << 16 ) |
                ( RoundFloat32ToU32( g * 255.0f ) << 8 ) |
                RoundFloat32ToU32( b * 255.0f );
    u8 *row = ( u8 * ) buffer->memory + minX * buffer->bytesPerPixel + minY * buffer->pitch;
    for ( int y = minY; y < maxY; ++y )
    {
        u32 *pixel = ( u32 * ) row;
        for ( int x = minX; x < maxX; ++x )
        {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

inline Tilemap *GetTileMap( World *world, int x, int y )
{
    if ( x >= 0 && x < world->countX && y >= 0 && y < world->countY )
    {
        return &world->tilemaps[ y * world->countX + x ];
    }
    return 0;
}

inline u32 GetTileValue( Tilemap *tilemap, int x, int y )
{
    return tilemap->tiles[ y * tilemap->countX + x ];
}

internal bool IsTilemapPointEmpty( Tilemap *tilemap, float32 x, float32 y )
{
    int playerTileX = TruncateFloat32ToS32( ( x - tilemap->positionX ) / tilemap->tileWidth );
    int playerTileY = TruncateFloat32ToS32( ( y - tilemap->positionY ) / tilemap->tileHeight );
    bool empty = false;
    if ( playerTileX >= 0 && playerTileX < tilemap->countX &&
         playerTileY >= 0 && playerTileY < tilemap->countY )
    {
        empty = GetTileValue( tilemap, playerTileX, playerTileY ) == 0;
    }
    return empty;
}

internal bool IsWorldPointEmpty( World *world, s32 tilemapX, s32 tilemapY, float32 x, float32 y )
{
    Tilemap *tilemap = GetTileMap( world, tilemapX, tilemapY );
    if ( tilemap )
    {
        return IsTilemapPointEmpty( tilemap, x, y );
    }
    return false;
}
extern "C" __declspec( dllexport )
GAME_UPDATE_AND_RENDER( GameUpdateAndRender )
{
    Assert( sizeof( Game_State ) <= memory->permanentStorageSize );
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;

    if ( !memory->isInitialized )
    {
        gameState->playerX = 400.0f;
        gameState->playerY = 400.0f;
        memory->isInitialized = true;
    }

    const int tilemapCountX = 17;
    const int tilemapCountY = 9;
    u32 tiles00[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    u32 tiles01[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 }
    };
    u32 tiles10[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    u32 tiles11[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };
    Tilemap tilemaps[ 2 ][ 2 ] = {};
    tilemaps[ 0 ][ 0 ].countX = tilemapCountX;
    tilemaps[ 0 ][ 0 ].countY = tilemapCountY;
    tilemaps[ 0 ][ 0 ].positionX = 0;
    tilemaps[ 0 ][ 0 ].positionY = 0;
    tilemaps[ 0 ][ 0 ].tileHeight = buffer->height / ( float32 ) tilemapCountY;
    tilemaps[ 0 ][ 0 ].tileWidth = buffer->width / ( float32 ) tilemapCountX;
    tilemaps[ 0 ][ 0 ].tiles = ( u32 * ) tiles00;

    tilemaps[ 0 ][ 1 ] = tilemaps[ 0 ][ 0 ];
    tilemaps[ 0 ][ 1 ].tiles = ( u32 * ) tiles01;

    tilemaps[ 1 ][ 0 ] = tilemaps[ 0 ][ 0 ];
    tilemaps[ 1 ][ 0 ].tiles = ( u32 * ) tiles10;

    tilemaps[ 1 ][ 1 ] = tilemaps[ 0 ][ 0 ];
    tilemaps[ 1 ][ 1 ].tiles = ( u32 * ) tiles11;

    World world = {};
    world.countX = 2;
    world.countY = 2;
    world.tilemaps = ( Tilemap * ) tilemaps;

    float32 playerWidth = 0.75 * tilemaps[ 0 ][ 0 ].tileWidth;
    float32 playerHeight = 0.75 * tilemaps[ 0 ][ 0 ].tileHeight;

    Tilemap *tilemap = &tilemaps[ 0 ][ 0 ];
    for ( int controllerIndex = 0; controllerIndex < ArrayCount( input->controllers ); ++controllerIndex )
    {
        Game_Controller_Input *controller = getController( input, controllerIndex );
        if ( controller->isAnalog )
        {
        }
        else
        {
            float32 dPlayerX = 0.0f;
            float32 dPlayerY = 0.0f;

            if ( controller->up.endedDown ) { dPlayerY = -1.0f; }
            if ( controller->down.endedDown ) { dPlayerY = 1.0f; }
            if ( controller->left.endedDown ) { dPlayerX = -1.0f; }
            if ( controller->right.endedDown ) { dPlayerX = 1.0f; }

            float32 newPlayerX = gameState->playerX + dPlayerX * input->dtForUpdate * 200;
            float32 newPlayerY = gameState->playerY + dPlayerY * input->dtForUpdate * 200;

            if ( IsTilemapPointEmpty( tilemap, newPlayerX + 0.5f * playerWidth, newPlayerY ) &&
                 IsTilemapPointEmpty( tilemap, newPlayerX - 0.5f * playerWidth, newPlayerY ) &&
                 IsTilemapPointEmpty( tilemap, newPlayerX, newPlayerY ) )
            {
                gameState->playerX = newPlayerX;
                gameState->playerY = newPlayerY;
            }
        }
    }

    DrawRectangle( buffer, 0, 0, buffer->width, buffer->height, 1.0f, 0.0f, 1.0f );
    for ( int row = 0; row < 9; ++row )
    {
        for ( int col = 0; col < 17; ++col )
        {
            float32 gray = GetTileValue( tilemap, col, row ) == 1 ? 1.0f : 0.5f;
            float32 minX = ( float32 ) col * tilemap->tileWidth;
            float32 minY = ( float32 ) row * tilemap->tileHeight;
            float32 maxX = minX + tilemap->tileWidth;
            float32 maxY = minY + tilemap->tileHeight;
            DrawRectangle( buffer, minX, minY, maxX, maxY, gray, gray, gray );
        }
    }

    DrawRectangle( buffer, gameState->playerX - playerWidth / 2, gameState->playerY - playerHeight,
                   gameState->playerX + playerWidth / 2, gameState->playerY, 0.75f, 0.2f, 0.9f );
}

extern "C" __declspec( dllexport )
GAME_GET_SOUND_SAMPLES( GameGetSoundSamples )
{
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;
    GameOutputSound( soundBuffer, gameState );
}

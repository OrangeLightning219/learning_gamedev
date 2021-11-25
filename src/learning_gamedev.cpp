#include "learning_gamedev.h"
#include "math_utils.h"
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
    if ( x >= 0 && x < world->tilemapCountX && y >= 0 && y < world->tilemapCountY )
    {
        return &world->tilemaps[ y * world->tilemapCountX + x ];
    }
    return 0;
}

inline u32 GetTileValue( World *world, Tilemap *tilemap, int x, int y )
{
    Assert( tilemap );
    Assert( x >= 0 && x < world->tileCountX && y >= 0 && y < world->tileCountY );
    return tilemap->tiles[ y * world->tileCountX + x ];
}

inline bool IsTilemapPointEmpty( World *world, Tilemap *tilemap, s32 x, s32 y )
{
    if ( !tilemap )
    {
        return false;
    }
    bool empty = false;
    if ( x >= 0 && x < world->tileCountX && y >= 0 && y < world->tileCountY )
    {
        empty = GetTileValue( world, tilemap, x, y ) == 0;
    }
    return empty;
}
inline void RecanonicalizeCoordinate( World *world, s32 tileCount, s32 *tilemap, s32 *tile, float32 *tileRelative )
{
    s32 offset = FloorFloat32ToS32( *tileRelative / world->tileSizeInMeters );
    *tile += offset;
    *tileRelative -= offset * world->tileSizeInMeters;

    Assert( *tileRelative >= 0 );
    Assert( *tileRelative < world->tileSizeInMeters );

    if ( *tile < 0 )
    {
        *tile = tileCount + *tile;
        --*tilemap;
    }

    if ( *tile >= tileCount )
    {
        *tile = *tile - tileCount;
        ++*tilemap;
    }
}
inline Canonical_Position RecanonicalizePosition( World *world, Canonical_Position position )
{
    Canonical_Position result = position;
    RecanonicalizeCoordinate( world, world->tileCountX, &result.tilemapX, &result.tileX, &result.x );
    RecanonicalizeCoordinate( world, world->tileCountY, &result.tilemapY, &result.tileY, &result.y );
    return result;
}

internal bool IsWorldPointEmpty( World *world, Canonical_Position position )
{
    Tilemap *tilemap = GetTileMap( world, position.tilemapX, position.tilemapY );
    return IsTilemapPointEmpty( world, tilemap, position.tileX, position.tileY );
}
extern "C" __declspec( dllexport )
GAME_UPDATE_AND_RENDER( GameUpdateAndRender )
{
    const int tilemapCountX = 16;
    const int tilemapCountY = 9;
    u32 tiles00[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 }
    };

    u32 tiles01[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 }
    };
    u32 tiles10[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    u32 tiles11[ tilemapCountY ][ tilemapCountX ] = {
        { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };
    Tilemap tilemaps[ 2 ][ 2 ] = {};
    tilemaps[ 0 ][ 0 ].tiles = ( u32 * ) tiles00;
    tilemaps[ 0 ][ 1 ].tiles = ( u32 * ) tiles01;
    tilemaps[ 1 ][ 0 ].tiles = ( u32 * ) tiles10;
    tilemaps[ 1 ][ 1 ].tiles = ( u32 * ) tiles11;

    World world = {};
    world.tileSizeInMeters = 1.4f;
    world.tileSizeInPixels = buffer->width / tilemapCountX;
    world.metersToPixels = ( float32 ) world.tileSizeInPixels / world.tileSizeInMeters;
    world.tilemapCountX = 2;
    world.tilemapCountY = 2;
    world.tilemaps = ( Tilemap * ) tilemaps;
    world.tileCountX = tilemapCountX;
    world.tileCountY = tilemapCountY;
    world.positionX = 0.0f;
    world.positionY = 0.0f;

    float32 playerHeight = 1.4f;
    float32 playerWidth = 0.55 * playerHeight;

    Assert( sizeof( Game_State ) <= memory->permanentStorageSize );
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;

    if ( !memory->isInitialized )
    {
        gameState->playerPosition.tilemapX = 0;
        gameState->playerPosition.tilemapY = 0;
        gameState->playerPosition.tileX = 4;
        gameState->playerPosition.tileY = 4;
        gameState->playerPosition.x = world.tileSizeInMeters / 2.0f;
        gameState->playerPosition.y = world.tileSizeInMeters / 2.0f;
        memory->isInitialized = true;
    }

    Tilemap *tilemap = GetTileMap( &world, gameState->playerPosition.tilemapX, gameState->playerPosition.tilemapY );
    Assert( tilemap );

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

            Canonical_Position playerPosition = gameState->playerPosition;
            playerPosition.x += dPlayerX * input->dtForUpdate * 10.0f;
            playerPosition.y += dPlayerY * input->dtForUpdate * 10.0f;
            playerPosition = RecanonicalizePosition( &world, playerPosition );
            Canonical_Position playerLeftPosition = playerPosition;
            playerLeftPosition.x -= 0.5f * playerWidth;
            playerLeftPosition = RecanonicalizePosition( &world, playerLeftPosition );
            Canonical_Position playerRightPosition = playerPosition;
            playerRightPosition.x += 0.5f * playerWidth;
            playerRightPosition = RecanonicalizePosition( &world, playerRightPosition );

            if ( IsWorldPointEmpty( &world, playerPosition ) &&
                 IsWorldPointEmpty( &world, playerLeftPosition ) &&
                 IsWorldPointEmpty( &world, playerRightPosition ) )
            {
                gameState->playerPosition = playerPosition;
            }
        }
    }

    DrawRectangle( buffer, 0, 0, buffer->width, buffer->height, 1.0f, 0.0f, 1.0f );
    for ( int row = 0; row < world.tileCountY; ++row )
    {
        for ( int col = 0; col < world.tileCountX; ++col )
        {
            float32 gray = GetTileValue( &world, tilemap, col, row ) == 1 ? 1.0f : 0.5f;
            if ( row == gameState->playerPosition.tileY && col == gameState->playerPosition.tileX )
            {
                gray = 0.3f;
            }
            float32 minX = world.positionX + ( float32 ) col * world.tileSizeInPixels;
            float32 minY = world.positionY + ( float32 ) row * world.tileSizeInPixels;
            float32 maxX = minX + world.tileSizeInPixels;
            float32 maxY = minY + world.tileSizeInPixels;
            DrawRectangle( buffer, minX, minY, maxX, maxY, gray, gray, gray );
        }
    }
    float32 playerX = world.positionX +
                      world.metersToPixels * gameState->playerPosition.x +
                      gameState->playerPosition.tileX * world.tileSizeInPixels;
    float32 playerY = world.positionY +
                      world.metersToPixels * gameState->playerPosition.y +
                      gameState->playerPosition.tileY * world.tileSizeInPixels;
    DrawRectangle( buffer, playerX - world.metersToPixels * playerWidth / 2, playerY - world.metersToPixels * playerHeight,
                   playerX + world.metersToPixels * playerWidth / 2, playerY, 0.75f, 0.2f, 0.9f );
}

extern "C" __declspec( dllexport )
GAME_GET_SOUND_SAMPLES( GameGetSoundSamples )
{
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;
    GameOutputSound( soundBuffer, gameState );
}

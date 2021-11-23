#include "learning_gamedev.h"
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
    Assert( x >= 0 && x < world->countX && y >= 0 && y < world->countY );
    return tilemap->tiles[ y * world->countX + x ];
}

inline bool IsTilemapPointEmpty( World *world, Tilemap *tilemap, s32 x, s32 y )
{
    if ( !tilemap )
    {
        return false;
    }
    bool empty = false;
    if ( x >= 0 && x < world->countX && y >= 0 && y < world->countY )
    {
        empty = GetTileValue( world, tilemap, x, y ) == 0;
    }
    return empty;
}

inline Canonical_Position GetCanonicalPosition( World *world, Raw_Position position )
{
    Canonical_Position result;
    float32 x = position.x - world->positionX;
    float32 y = position.y - world->positionY;
    result.tileX = FloorFloat32ToS32( x / world->tileWidth );
    result.tileY = FloorFloat32ToS32( y / world->tileHeight );
    result.tilemapX = position.tilemapX;
    result.tilemapY = position.tilemapY;

    result.x = x - result.tileX * world->tileWidth;
    result.y = y - result.tileY * world->tileHeight;

    if ( result.tileX < 0 )
    {
        result.tileX = world->countX + result.tileX;
        --result.tilemapX;
    }

    if ( result.tileY < 0 )
    {
        result.tileY = world->countY + result.tileY;
        --result.tilemapY;
    }

    if ( result.tileX >= world->countX )
    {
        result.tileX = result.tileX - world->countX;
        ++result.tilemapX;
    }

    if ( result.tileY >= world->countY )
    {
        result.tileY = result.tileY - world->countY;
        ++result.tilemapY;
    }
    return result;
}

internal bool IsWorldPointEmpty( World *world, Raw_Position rawPosition )
{
    Canonical_Position position = GetCanonicalPosition( world, rawPosition );
    Tilemap *tilemap = GetTileMap( world, position.tilemapX, position.tilemapY );
    return IsTilemapPointEmpty( world, tilemap, position.tileX, position.tileY );
}
extern "C" __declspec( dllexport )
GAME_UPDATE_AND_RENDER( GameUpdateAndRender )
{
    Assert( sizeof( Game_State ) <= memory->permanentStorageSize );
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;

    if ( !memory->isInitialized )
    {
        gameState->playerTilemapX = 1;
        gameState->playerTilemapY = 1;
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
    World world = {};
    world.tilemapCountX = 2;
    world.tilemapCountY = 2;
    world.tilemaps = ( Tilemap * ) tilemaps;
    world.countX = tilemapCountX;
    world.countY = tilemapCountY;
    world.positionX = 0.0f;
    world.positionY = 0.0f;
    world.tileHeight = buffer->height / ( float32 ) tilemapCountY;
    world.tileWidth = buffer->width / ( float32 ) tilemapCountX;

    tilemaps[ 0 ][ 0 ].tiles = ( u32 * ) tiles00;
    tilemaps[ 0 ][ 1 ].tiles = ( u32 * ) tiles01;
    tilemaps[ 1 ][ 0 ].tiles = ( u32 * ) tiles10;
    tilemaps[ 1 ][ 1 ].tiles = ( u32 * ) tiles11;

    float32 playerWidth = 0.75 * world.tileWidth;
    float32 playerHeight = 0.75 * world.tileHeight;

    Tilemap *tilemap = GetTileMap( &world, gameState->playerTilemapX, gameState->playerTilemapY );
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

            float32 newPlayerX = gameState->playerX + dPlayerX * input->dtForUpdate * 200;
            float32 newPlayerY = gameState->playerY + dPlayerY * input->dtForUpdate * 200;

            Raw_Position playerPosition = { gameState->playerTilemapX, gameState->playerTilemapY, newPlayerX, newPlayerY };
            Raw_Position playerLeftPosition = { gameState->playerTilemapX, gameState->playerTilemapY, newPlayerX - 0.5f * playerWidth, newPlayerY };
            Raw_Position playerRightPosition = { gameState->playerTilemapX, gameState->playerTilemapY, newPlayerX + 0.5f * playerWidth, newPlayerY };
            if ( IsWorldPointEmpty( &world, playerPosition ) &&
                 IsWorldPointEmpty( &world, playerLeftPosition ) &&
                 IsWorldPointEmpty( &world, playerRightPosition ) )
            {
                Canonical_Position canonicalPlayerPosition = GetCanonicalPosition( &world, playerPosition );
                gameState->playerTilemapX = canonicalPlayerPosition.tilemapX;
                gameState->playerTilemapY = canonicalPlayerPosition.tilemapY;
                gameState->playerX = world.positionX + world.tileWidth * canonicalPlayerPosition.tileX + canonicalPlayerPosition.x;
                gameState->playerY = world.positionY + world.tileHeight * canonicalPlayerPosition.tileY + canonicalPlayerPosition.y;
            }
        }
    }

    DrawRectangle( buffer, 0, 0, buffer->width, buffer->height, 1.0f, 0.0f, 1.0f );
    for ( int row = 0; row < world.countY; ++row )
    {
        for ( int col = 0; col < world.countX; ++col )
        {
            float32 gray = GetTileValue( &world, tilemap, col, row ) == 1 ? 1.0f : 0.5f;
            float32 minX = world.positionX + ( float32 ) col * world.tileWidth;
            float32 minY = world.positionY + ( float32 ) row * world.tileHeight;
            float32 maxX = minX + world.tileWidth;
            float32 maxY = minY + world.tileHeight;
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

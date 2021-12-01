#include "utils.h"
#include "math_utils.h"
#include "learning_gamedev.h"
#include "tilemap.cpp"

#ifndef UNITY_BUILD
#endif

internal void GameOutputSound( Game_Sound_Output_Buffer *buffer )
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

internal void InitializeArena( Memory_Arena *arena, memory_index size, u8 *base )
{
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

#define PushStruct( arena, type )       ( type * ) _PushSize( arena, sizeof( type ) )
#define PushArray( arena, count, type ) ( type * ) _PushSize( arena, count * sizeof( type ) )
internal void *_PushSize( Memory_Arena *arena, memory_index size )
{
    Assert( arena->used + size <= arena->size );
    void *result = arena->base + arena->used;
    arena->used += size;
    return result;
}

extern "C" __declspec( dllexport )
GAME_UPDATE_AND_RENDER( GameUpdateAndRender )
{
    float32 playerHeight = 1.4f;
    float32 playerWidth = 0.55f * playerHeight;

    Assert( sizeof( Game_State ) <= memory->permanentStorageSize );
    Game_State *gameState = ( Game_State * ) memory->permanentStorage;

    if ( !memory->isInitialized )
    {
        gameState->playerPosition.tileX = 4;
        gameState->playerPosition.tileY = 4;
        gameState->playerPosition.x = 0.0f; //tilemap.tileSizeInMeters / 2.0f;
        gameState->playerPosition.y = 0.0f; //tilemap.tileSizeInMeters / 2.0f;

        InitializeArena( &gameState->worldArena, memory->permanentStorageSize - sizeof( Game_State ),
                         ( u8 * ) memory->permanentStorage + sizeof( Game_State ) );
        gameState->world = PushStruct( &gameState->worldArena, World );
        gameState->world->tilemap = PushStruct( &gameState->worldArena, Tilemap );

        Tilemap *tilemap = gameState->world->tilemap;
        tilemap->chunkShift = 8;
        tilemap->chunkMask = ( 1 << tilemap->chunkShift ) - 1;
        tilemap->chunkSize = 1 << tilemap->chunkShift;
        tilemap->tileSizeInMeters = 1.4f;
        tilemap->tileSizeInPixels = buffer->width / 16;
        tilemap->metersToPixels = ( float32 ) tilemap->tileSizeInPixels / tilemap->tileSizeInMeters;
        tilemap->tileChunkCountX = 4;
        tilemap->tileChunkCountY = 4;

        tilemap->tileChunks = PushArray( &gameState->worldArena, tilemap->tileChunkCountX * tilemap->tileChunkCountY, Tile_Chunk );
        for ( u32 chunkY = 0; chunkY < tilemap->tileChunkCountY; ++chunkY )
        {
            for ( u32 chunkX = 0; chunkX < tilemap->tileChunkCountX; ++chunkX )
            {
                tilemap->tileChunks[ chunkY * tilemap->tileChunkCountX + chunkX ].tiles = PushArray( &gameState->worldArena,
                                                                                                     tilemap->chunkSize * tilemap->chunkSize,
                                                                                                     u32 );
            }
        }

        const int tilemapCountX = 16;
        const int tilemapCountY = 9;
        for ( u32 screenY = 0; screenY < 32; ++screenY )
        {
            for ( u32 screenX = 0; screenX < 32; ++screenX )
            {
                for ( u32 tileY = 0; tileY < tilemapCountY; ++tileY )
                {
                    for ( u32 tileX = 0; tileX < tilemapCountX; ++tileX )
                    {
                        u32 x = screenX * tilemapCountX + tileX;
                        u32 y = screenY * tilemapCountY + tileY;
                        SetTileValue( &gameState->worldArena, gameState->world->tilemap, x, y, tileY == tileX && tileY % 2 ? 1 : 0 );
                    }
                }
            }
        }

        memory->isInitialized = true;
    }

    World *world = gameState->world;
    Tilemap *tilemap = world->tilemap;
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

            if ( controller->up.endedDown ) { dPlayerY = 1.0f; }
            if ( controller->down.endedDown ) { dPlayerY = -1.0f; }
            if ( controller->left.endedDown ) { dPlayerX = -1.0f; }
            if ( controller->right.endedDown ) { dPlayerX = 1.0f; }

            Tilemap_Position playerPosition = gameState->playerPosition;
            playerPosition.x += dPlayerX * input->dtForUpdate * 60.0f;
            playerPosition.y += dPlayerY * input->dtForUpdate * 60.0f;
            playerPosition = RecanonicalizePosition( tilemap, playerPosition );
            Tilemap_Position playerLeftPosition = playerPosition;
            playerLeftPosition.x -= 0.5f * playerWidth;
            playerLeftPosition = RecanonicalizePosition( tilemap, playerLeftPosition );
            Tilemap_Position playerRightPosition = playerPosition;
            playerRightPosition.x += 0.5f * playerWidth;
            playerRightPosition = RecanonicalizePosition( tilemap, playerRightPosition );

            if ( IsTilemapPointEmpty( tilemap, playerPosition ) &&
                 IsTilemapPointEmpty( tilemap, playerLeftPosition ) &&
                 IsTilemapPointEmpty( tilemap, playerRightPosition ) )
            {
                gameState->playerPosition = playerPosition;
            }
        }
    }

    float32 screenCenterX = ( float32 ) buffer->width / 2.0f;
    float32 screenCenterY = ( float32 ) buffer->height / 2.0f;
    DrawRectangle( buffer, 0.0f, 0.0f, ( float32 ) buffer->width, ( float32 ) buffer->height, 1.0f, 0.0f, 1.0f );
    for ( s32 relativeRow = -6; relativeRow < 6; ++relativeRow )
    {
        for ( s32 relativeCol = -9; relativeCol < +9; ++relativeCol )
        {
            u32 row = gameState->playerPosition.tileY + relativeRow;
            u32 col = gameState->playerPosition.tileX + relativeCol;
            float32 gray = GetTileValue( tilemap, col, row ) == 1 ? 1.0f : 0.5f;
            if ( row == gameState->playerPosition.tileY && col == gameState->playerPosition.tileX )
            {
                gray = 0.3f;
            }
            float32 centerX = screenCenterX + ( float32 ) relativeCol * tilemap->tileSizeInPixels - tilemap->metersToPixels * gameState->playerPosition.x;
            float32 centerY = screenCenterY - ( float32 ) relativeRow * tilemap->tileSizeInPixels + tilemap->metersToPixels * gameState->playerPosition.y;
            float32 minX = centerX - 0.5f * tilemap->tileSizeInPixels;
            float32 minY = centerY - 0.5f * tilemap->tileSizeInPixels;
            float32 maxX = centerX + 0.5f * tilemap->tileSizeInPixels;
            float32 maxY = centerY + 0.5f * tilemap->tileSizeInPixels;
            DrawRectangle( buffer, minX, minY, maxX, maxY, gray, gray, gray );
        }
    }
    float32 playerX = screenCenterX;
    float32 playerY = screenCenterY;
    DrawRectangle( buffer, playerX - tilemap->metersToPixels * playerWidth / 2, playerY - tilemap->metersToPixels * playerHeight,
                   playerX + tilemap->metersToPixels * playerWidth / 2, playerY, 0.75f, 0.2f, 0.9f );
}

extern "C" __declspec( dllexport )
GAME_GET_SOUND_SAMPLES( GameGetSoundSamples )
{
    GameOutputSound( soundBuffer );
}

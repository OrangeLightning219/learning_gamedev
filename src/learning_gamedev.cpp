#include "utils.h"
#include "math_utils.h"
#include "learning_gamedev.h"
#include "tilemap.cpp"
#include "random.h"
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

#pragma pack( push, 1 )
struct Bitmap_Header
{
    u16 fileType;
    u32 fileSize;
    u16 reserved1;
    u16 reserved2;
    u32 bitmapOffset;
    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bitsPerPixel;
};
#pragma pack( pop )

internal u32 *DebugLoadBMP( Thread_Context *thread, debug_platform_read_entire_file *ReadEntireFile, char *filename )
{
    u32 *pixels = 0;
    Debug_Read_File_Result readResult = ReadEntireFile( thread, filename );
    if ( readResult.contentSize > 0 )
    {
        Bitmap_Header *header = ( Bitmap_Header * ) readResult.content;
        pixels = ( u32 * ) ( ( u8 * ) readResult.content + header->bitmapOffset );
        int a = 0;
    }

    return pixels;
}

internal void InitializeArena( Memory_Arena *arena, memory_index size, u8 *base )
{
    arena->size = size;
    arena->base = base;
    arena->used = 0;
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
        gameState->pixelPointer = DebugLoadBMP( thread, memory->DebugPlatformReadEntireFile, "test.bmp" );
        gameState->playerPosition.tileX = 4;
        gameState->playerPosition.tileY = 4;
        gameState->playerPosition.x = 0.0f; //tilemap.tileSizeInMeters / 2.0f;
        gameState->playerPosition.y = 0.0f; //tilemap.tileSizeInMeters / 2.0f;

        InitializeArena( &gameState->worldArena, memory->permanentStorageSize - sizeof( Game_State ),
                         ( u8 * ) memory->permanentStorage + sizeof( Game_State ) );
        gameState->world = PushStruct( &gameState->worldArena, World );
        gameState->world->tilemap = PushStruct( &gameState->worldArena, Tilemap );

        Tilemap *tilemap = gameState->world->tilemap;
        tilemap->chunkShift = 4;
        tilemap->chunkMask = ( 1 << tilemap->chunkShift ) - 1;
        tilemap->chunkSize = 1 << tilemap->chunkShift;
        tilemap->tileSizeInMeters = 1.4f;
        tilemap->tileChunkCountX = 128;
        tilemap->tileChunkCountY = 128;
        tilemap->tileChunkCountZ = 2;

        tilemap->tileChunks = PushArray( &gameState->worldArena,
                                         tilemap->tileChunkCountX * tilemap->tileChunkCountY * tilemap->tileChunkCountZ,
                                         Tile_Chunk );

        u32 tilemapCountX = 16;
        u32 tilemapCountY = 9;
        u32 screenX = 0;
        u32 screenY = 0;
        u32 randomNumberIndex = 10;

        bool doorLeft = false;
        bool doorRight = false;
        bool doorTop = false;
        bool doorBottom = false;
        bool doorUp = false;
        bool doorDown = false;
        u32 z = 0;

        for ( u32 screenIndex = 0; screenIndex < 100; ++screenIndex )
        {
            Assert( randomNumberIndex < ArrayCount( randomNumberTable ) );
            u32 randomChoice;
            if ( doorUp || doorDown )
            {
                randomChoice = randomNumberTable[ randomNumberIndex++ ] % 2;
            }
            else
            {
                randomChoice = randomNumberTable[ randomNumberIndex++ ] % 3;
            }

            bool createdZDoor = false;
            if ( randomChoice == 2 )
            {
                createdZDoor = true;
                if ( z == 0 )
                {
                    doorUp = true;
                }
                else
                {
                    doorDown = true;
                }
            }
            else if ( randomChoice == 1 )
            {
                doorRight = true;
            }
            else
            {
                doorTop = true;
            }
            for ( u32 tileY = 0; tileY < tilemapCountY; ++tileY )
            {
                for ( u32 tileX = 0; tileX < tilemapCountX; ++tileX )
                {
                    u32 x = screenX * tilemapCountX + tileX;
                    u32 y = screenY * tilemapCountY + tileY;
                    u32 tileValue = 1;
                    if ( tileX == 0 && ( !doorLeft || tileY != tilemapCountY / 2 ) )
                    {
                        tileValue = 2;
                    }
                    if ( tileX == tilemapCountX - 1 && ( !doorRight || tileY != tilemapCountY / 2 ) )
                    {
                        tileValue = 2;
                    }

                    if ( tileY == 0 && ( !doorBottom || tileX != tilemapCountX / 2 ) )
                    {
                        tileValue = 2;
                    }

                    if ( tileY == tilemapCountY - 1 && ( !doorTop || tileX != tilemapCountX / 2 ) )
                    {
                        tileValue = 2;
                    }
                    if ( tileX == 5 && tileY == 5 )
                    {
                        if ( doorUp )
                        {
                            tileValue = 3;
                        }
                        if ( doorDown )
                        {
                            tileValue = 4;
                        }
                    }
                    SetTileValue( &gameState->worldArena, gameState->world->tilemap, x, y, z, tileValue );
                }
            }

            if ( createdZDoor )
            {
                doorDown = !doorDown;
                doorUp = !doorUp;
            }
            else
            {
                doorUp = false;
                doorDown = false;
            }
            doorLeft = doorRight;
            doorRight = false;
            doorBottom = doorTop;
            doorTop = false;
            if ( randomChoice == 2 )
            {
                if ( z == 0 )
                {
                    z = 1;
                }
                else
                {
                    z = 0;
                }
            }
            else if ( randomChoice == 1 )
            {
                screenX += 1;
            }
            else
            {
                screenY += 1;
            }
        }
        memory->isInitialized = true;
    }
    World *world = gameState->world;
    Tilemap *tilemap = world->tilemap;

    s32 tileSizeInPixels;
    float32 metersToPixels;
    tileSizeInPixels = buffer->width / 16;
    metersToPixels = ( float32 ) tileSizeInPixels / tilemap->tileSizeInMeters;

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
            playerPosition.x += dPlayerX * input->dtForUpdate * 10.0f;
            playerPosition.y += dPlayerY * input->dtForUpdate * 10.0f;
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
                if ( !AreOnSameTile( &gameState->playerPosition, &playerPosition ) )
                {
                    u32 tileValue = GetTileValue( tilemap, playerPosition );
                    if ( tileValue == 3 )
                    {
                        ++playerPosition.tileZ;
                    }
                    else if ( tileValue == 4 )
                    {
                        --playerPosition.tileZ;
                    }
                }
                gameState->playerPosition = playerPosition;
            }
        }
    }

    float32 screenCenterX = ( float32 ) buffer->width / 2.0f;
    float32 screenCenterY = ( float32 ) buffer->height / 2.0f;
    DrawRectangle( buffer, 0.0f, 0.0f, ( float32 ) buffer->width, ( float32 ) buffer->height, 1.0f, 0.0f, 1.0f );
    for ( s32 relativeRow = -6; relativeRow < 6; ++relativeRow )
    {
        for ( s32 relativeCol = -15; relativeCol < 15; ++relativeCol )
        {
            u32 row = gameState->playerPosition.tileY + relativeRow;
            u32 col = gameState->playerPosition.tileX + relativeCol;
            u32 tileValue = GetTileValue( tilemap, col, row, gameState->playerPosition.tileZ );
            if ( tileValue > 0 )
            {
                float32 gray = tileValue == 2 ? 1.0f : 0.5f;
                if ( tileValue > 2 ) { gray = 0.2f; }
                if ( row == gameState->playerPosition.tileY && col == gameState->playerPosition.tileX )
                {
                    gray = 0.3f;
                }
                float32 centerX = screenCenterX + ( float32 ) relativeCol * tileSizeInPixels - metersToPixels * gameState->playerPosition.x;
                float32 centerY = screenCenterY - ( float32 ) relativeRow * tileSizeInPixels + metersToPixels * gameState->playerPosition.y;
                float32 minX = centerX - 0.5f * tileSizeInPixels;
                float32 minY = centerY - 0.5f * tileSizeInPixels;
                float32 maxX = centerX + 0.5f * tileSizeInPixels;
                float32 maxY = centerY + 0.5f * tileSizeInPixels;
                DrawRectangle( buffer, minX, minY, maxX, maxY, gray, gray, gray );
            }
        }
    }
    float32 playerX = screenCenterX;
    float32 playerY = screenCenterY;
    DrawRectangle( buffer, playerX - metersToPixels * playerWidth / 2, playerY - metersToPixels * playerHeight,
                   playerX + metersToPixels * playerWidth / 2, playerY, 0.75f, 0.2f, 0.9f );

    u32 *source = gameState->pixelPointer;
    u32 *destination = ( u32 * ) buffer->memory;
    for ( s32 y = 0; y < buffer->height; ++y )
    {
        for ( s32 x = 0; x < buffer->width; ++x )
        {
            *destination++ = *source++;
        }
    }
}

extern "C" __declspec( dllexport )
GAME_GET_SOUND_SAMPLES( GameGetSoundSamples )
{
    GameOutputSound( soundBuffer );
}

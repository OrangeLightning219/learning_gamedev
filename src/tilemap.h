#ifndef TILEMAP_H
#define TILEMAP_H

#ifndef UNITY_BUILD
    #include "utils.h"
    #include "math_utils.h"
#endif

struct Tilemap_Position
{
    u32 tileX;
    u32 tileY;
    // relative to the tile
    float32 x;
    float32 y;
};

struct Tile_Chunk_Position
{
    u32 tileChunkX;
    u32 tileChunkY;
    u32 tileX;
    u32 tileY;
};

struct Tile_Chunk
{
    u32 *tiles;
};

struct Tilemap
{
    u32 chunkShift;
    u32 chunkMask;
    u32 chunkSize;

    float32 tileSizeInMeters;
    s32 tileSizeInPixels;
    float32 metersToPixels;

    u32 tileChunkCountX;
    u32 tileChunkCountY;
    Tile_Chunk *tileChunks;
};

#endif

#ifndef UNITY_BUILD
    #include "learning_gamedev.h"
#endif
inline Tile_Chunk *GetTileChunk( Tilemap *tilemap, u32 x, u32 y, u32 z )
{
    if ( x >= 0 && x < tilemap->tileChunkCountX &&
         y >= 0 && y < tilemap->tileChunkCountY &&
         z >= 0 && z < tilemap->tileChunkCountZ )
    {
        return &tilemap->tileChunks[ z * tilemap->tileChunkCountY * tilemap->tileChunkCountX +
                                     y * tilemap->tileChunkCountX +
                                     x ];
    }
    return 0;
}

inline u32 GetTileValueUnchecked( Tilemap *tilemap, Tile_Chunk *tileChunk, u32 x, u32 y )
{
    Assert( tileChunk );
    Assert( x < tilemap->chunkSize && y < tilemap->chunkSize );
    return tileChunk->tiles[ y * tilemap->chunkSize + x ];
}

inline void SetTileValueUnchecked( Tilemap *tilemap, Tile_Chunk *tileChunk, u32 x, u32 y, u32 value )
{
    Assert( tileChunk );
    Assert( x < tilemap->chunkSize && y < tilemap->chunkSize );
    tileChunk->tiles[ y * tilemap->chunkSize + x ] = value;
}

inline u32 GetTileValue( Tilemap *tilemap, Tile_Chunk *tileChunk, u32 x, u32 y )
{
    if ( !tileChunk || !tileChunk->tiles )
    {
        return 0;
    }
    return GetTileValueUnchecked( tilemap, tileChunk, x, y );
}

inline void
SetTileValue( Tilemap *tilemap, Tile_Chunk *tileChunk, u32 x, u32 y, u32 value )
{
    if ( !tileChunk || !tileChunk->tiles )
    {
        return;
    }
    SetTileValueUnchecked( tilemap, tileChunk, x, y, value );
}

inline void RecanonicalizeCoordinate( Tilemap *tilemap, u32 *tile, float32 *tileRelative )
{
    s32 offset = RoundFloat32ToS32( *tileRelative / tilemap->tileSizeInMeters );
    *tile += offset;
    *tileRelative -= offset * tilemap->tileSizeInMeters;

    Assert( *tileRelative >= -0.5f * tilemap->tileSizeInMeters );
    Assert( *tileRelative <= 0.5f * tilemap->tileSizeInMeters );
}

inline Tilemap_Position RecanonicalizePosition( Tilemap *tilemap, Tilemap_Position position )
{
    Tilemap_Position result = position;
    RecanonicalizeCoordinate( tilemap, &result.tileX, &result.x );
    RecanonicalizeCoordinate( tilemap, &result.tileY, &result.y );
    return result;
}

inline Tile_Chunk_Position GetChunkPosition( Tilemap *tilemap, u32 tileX, u32 tileY, u32 tileZ )
{
    Tile_Chunk_Position result = {};
    result.tileChunkX = tileX >> tilemap->chunkShift;
    result.tileChunkY = tileY >> tilemap->chunkShift;
    result.tileChunkZ = tileZ;
    result.tileX = tileX & tilemap->chunkMask;
    result.tileY = tileY & tilemap->chunkMask;
    return result;
}

internal u32 GetTileValue( Tilemap *tilemap, u32 tileX, u32 tileY, u32 tileZ )
{
    Tile_Chunk_Position chunkPosition = GetChunkPosition( tilemap, tileX, tileY, tileZ );
    Tile_Chunk *chunk = GetTileChunk( tilemap, chunkPosition.tileChunkX, chunkPosition.tileChunkY, chunkPosition.tileChunkZ );
    u32 tileValue = GetTileValue( tilemap, chunk, chunkPosition.tileX, chunkPosition.tileY );
    return tileValue;
}

inline u32 GetTileValue( Tilemap *tilemap, Tilemap_Position position )
{
    return GetTileValue( tilemap, position.tileX, position.tileY, position.tileZ );
}

internal bool IsTilemapPointEmpty( Tilemap *tilemap, Tilemap_Position position )
{
    u32 tileValue = GetTileValue( tilemap, position );
    return tileValue == 1 || tileValue == 3 || tileValue == 4;
}

internal void SetTileValue( Memory_Arena *arena, Tilemap *tilemap, u32 x, u32 y, u32 z, u32 value )
{
    Tile_Chunk_Position chunkPosition = GetChunkPosition( tilemap, x, y, z );
    Tile_Chunk *chunk = GetTileChunk( tilemap, chunkPosition.tileChunkX, chunkPosition.tileChunkY, chunkPosition.tileChunkZ );
    Assert( chunk );

    if ( !chunk->tiles )
    {
        u32 tileCount = tilemap->chunkSize * tilemap->chunkSize;
        chunk->tiles = PushArray( arena, tileCount, u32 );
        for ( u32 tileIndex = 0; tileIndex < tileCount; ++tileIndex )
        {
            chunk->tiles[ tileIndex ] = 1;
        }
    }

    SetTileValue( tilemap, chunk, chunkPosition.tileX, chunkPosition.tileY, value );
}

inline bool AreOnSameTile( Tilemap_Position *a, Tilemap_Position *b )
{
    return a->tileX == b->tileX && a->tileY == b->tileY && a->tileZ == b->tileZ;
}

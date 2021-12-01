#ifndef UNITY_BUILD
    #include "learning_gamedev.h"
#endif
inline Tile_Chunk *GetTileChunk( Tilemap *tilemap, u32 x, u32 y )
{
    if ( x >= 0 && x < tilemap->tileChunkCountX && y >= 0 && y < tilemap->tileChunkCountY )
    {
        return &tilemap->tileChunks[ y * tilemap->tileChunkCountX + x ];
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
    if ( !tileChunk )
    {
        return 0;
    }
    return GetTileValueUnchecked( tilemap, tileChunk, x, y );
}

inline void SetTileValue( Tilemap *tilemap, Tile_Chunk *tileChunk, u32 x, u32 y, u32 value )
{
    if ( !tileChunk )
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

inline Tile_Chunk_Position GetChunkPosition( Tilemap *tilemap, u32 tileX, u32 tileY )
{
    Tile_Chunk_Position result = {};
    result.tileChunkX = tileX >> tilemap->chunkShift;
    result.tileChunkY = tileY >> tilemap->chunkShift;
    result.tileX = tileX & tilemap->chunkMask;
    result.tileY = tileY & tilemap->chunkMask;
    return result;
}

internal u32 GetTileValue( Tilemap *tilemap, u32 tileX, u32 tileY )
{
    Tile_Chunk_Position chunkPosition = GetChunkPosition( tilemap, tileX, tileY );
    Tile_Chunk *chunk = GetTileChunk( tilemap, chunkPosition.tileChunkX, chunkPosition.tileChunkY );
    u32 tileValue = GetTileValue( tilemap, chunk, chunkPosition.tileX, chunkPosition.tileY );
    return tileValue;
}

internal bool IsTilemapPointEmpty( Tilemap *tilemap, Tilemap_Position position )
{
    return GetTileValue( tilemap, position.tileX, position.tileY ) == 0;
}

internal void SetTileValue( Memory_Arena *arena, Tilemap *tilemap, u32 x, u32 y, u32 value )
{
    Tile_Chunk_Position chunkPosition = GetChunkPosition( tilemap, x, y );
    Tile_Chunk *chunk = GetTileChunk( tilemap, chunkPosition.tileChunkX, chunkPosition.tileChunkY );
    Assert( chunk );
    SetTileValue( tilemap, chunk, chunkPosition.tileX, chunkPosition.tileY, value );
}

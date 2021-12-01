#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <math.h>
#ifndef UNITY_BUILD
    #include "utils.h"
#endif

inline s32 RoundFloat32ToS32( float32 value )
{
    return ( s32 ) roundf( value );
}

inline u32 RoundFloat32ToU32( float32 value )
{
    return ( u32 ) roundf( value );
}

inline s32 TruncateFloat32ToS32( float32 value )
{
    return ( s32 ) value;
}
inline s32 FloorFloat32ToS32( float32 value )
{
    return ( s32 ) floorf( value );
}

inline float32 Sin( float32 value )
{
    return sinf( value );
}

inline float32 Cos( float32 value )
{
    return cosf( value );
}

inline float32 Atan2( float32 y, float32 x )
{
    return atan2f( y, x );
}

#endif /* MATH_UTILS_H */

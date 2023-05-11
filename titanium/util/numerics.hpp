#pragma once

#include <stdint.h>
#include <stdlib.h> // for size_t
#include <math.h> // for maths funcs (pow, log, etc)
#include <limits>

/*

NOTE: NOT IN A NAMESPACE! these are largely shorthand definitions for int types
namespacing them would kind of ruin the whole point of them being shorthand! so we don't 

*/

using u8 = uint8_t;
using byte = u8;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

static_assert(sizeof(f32) * 8 == 32, "Bad size for type f32! (should be 32 bits/4 bytes)");
static_assert(sizeof(f64) * 8 == 64, "Bad size for type f64! (should be 64 bits/8 bytes)");

// does this suck? unsure
#define minof( type ) std::numeric_limits<type>::min()
#define maxof( type ) std::numeric_limits<type>::max()

// TODO: is this the right header for these types? could be in a more physics-focused header perhaps
/*

DOUBLE_PRECISION_POSITIONS controls whether we should use 64-bit floats (doubles) for storing positions
Not needed by default, but could be useful later

*/
#ifndef DOUBLE_PRECISION_POSITIONS
    #define DOUBLE_PRECISION_POSITIONS 0
#endif // #ifndef DOUBLE_PRECISION_POSITIONS

/*

fPositional is the floating point type we use for positions
to make it easier if we ever wanna transition to double precision floats for object positions

*/
using fPositional = 
#if DOUBLE_PRECISION_POSITIONS
    f64
#else
    f32
#endif // #if DOUBLE_PRECISION_POSITIONS
;
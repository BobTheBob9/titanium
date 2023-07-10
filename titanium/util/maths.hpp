#pragma once

#include "titanium/util/numerics.hpp"
#include "titanium/util/template_constraints.hpp"

namespace util::maths
{
    template<typename T> requires util::templateconstraints::Numeric<T>
    bool NumberWithinRange( const T tValue, const T tBegin, const T tEnd )
    {
        return tValue >= tBegin && tValue < tEnd;
    }

    // TODO: could split up this header into multiple files
    // TODO: is this the right header for these types? could be in a more physics-focused header perhaps
    /*
     *  DOUBLE_PRECISION_DEFAULT_VECTOR controls whether we should use 64-bit floats (doubles) as our default vector type
     *  Not needed by default, but could be useful later
     */
    #ifndef DOUBLE_PRECISION_DEFAULT_VECTOR
        #define DOUBLE_PRECISION_DEFAULT_VECTOR 0
    #endif // #ifndef DOUBLE_PRECISION_POSITIONS

    template<typename T> requires util::templateconstraints::Arithmetic<T>
    struct Vec3 
    {
        T x, y, z;
    };

    using Vec3i = Vec3<i32>;

    #if DOUBLE_PRECISION_DEFAULT_VECTOR 
        using Vec3f = Vec3<f64>;
    #else // #if DOUBLE_PRECISION_DEFAULT_VECTOR
        using Vec3f = Vec3<f32>;
    #endif // #if DOUBLE_PRECISION_DEFAULT_VECTOR



}

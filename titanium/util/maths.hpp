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

    template<typename T> requires util::templateconstraints::Arithmetic<T>
    struct Vec2 
    {
        T x, y;
    };

    static_assert( sizeof( Vec2<i32> ) == sizeof( i32 ) * 2 );
    static_assert( sizeof( Vec2<f32> ) == sizeof( f32 ) * 2 );

    template<typename T> requires util::templateconstraints::Arithmetic<T>
    struct Vec3 
    {
        T x, y, z;
    };

    static_assert( sizeof( Vec3<i32> ) == sizeof( i32 ) * 3 );
    static_assert( sizeof( Vec3<f32> ) == sizeof( f32 ) * 3 );
}

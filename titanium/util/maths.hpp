#pragma once

#include "titanium/util/numerics.hpp"
#include "titanium/util/template_constraints.hpp"

namespace util::maths
{
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

    template<typename T> requires util::templateconstraints::Arithmetic<T>
    using Matrix4x4 = T[ 4 ][ 4 ]; 

    Matrix4x4 Matrix4x4_Multiply( const Matrix4x4 mat4x4First, const Matrix4x4 mat4x4Second )
    {
        //return 
    }

    // TODO: would be good if we had a (compile time!!!) way to take a variable number of args to these
    // TODO: is this even faster branchless? need a way to check assembly of func
    template<typename T> requires util::templateconstraints::Numeric<T>
    T Min( const T nFirst, const T nSecond )
    {
        return ( nFirst * ( nFirst < nSecond ) ) + ( nSecond * ( nSecond <= nFirst ) ); 
    }

    template<typename T> requires util::templateconstraints::Numeric<T>
    T Max( const T nFirst, const T nSecond )
    {
        return ( nFirst * ( nFirst > nSecond ) ) + ( nSecond * ( nSecond >= nFirst ) ); 
    }

    template<typename T> requires util::templateconstraints::Numeric<T>
    bool WithinRange( const T tValue, const T tBegin, const T tEnd )
    {
        return tValue >= tBegin && tValue < tEnd;
    }
}

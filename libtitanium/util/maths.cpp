#include "maths.hpp"

namespace util::maths
{
    Matrix4x4 Matrix4x4::FromPosition( const Vec3<f32> fvPosition )
    {
        return { .flMatrices = {
            { 1.f,          0.f,          0.f,          1.f },
            { 0.f,          1.f,          0.f,          1.f },
            { 0.f,          0.f,          1.f,          1.f },
            { fvPosition.x, fvPosition.y, fvPosition.z, 1.f }
        } };
    }

    Matrix4x4 Matrix4x4::FromAngles( const Vec3Angle<f32> fvAngles )
    {
        const f32 yawCos = cosf( fvAngles.yaw );
        const f32 yawSin = sinf( fvAngles.yaw );
        const f32 pitchCos = cosf( fvAngles.pitch );
        const f32 pitchSin = sinf( fvAngles.pitch );
        const f32 rollCos = cosf( fvAngles.roll );
        const f32 rollSin = sinf( fvAngles.roll );

        return { .flMatrices = {
            { ( yawCos * pitchCos ) + ( yawSin * pitchSin * rollSin ), pitchCos * rollSin, ( -yawSin * rollCos ) + ( yawCos * pitchSin * rollCos ), 0.f },
            { ( -yawCos * rollSin ) + ( yawSin * pitchSin * rollCos ), rollCos * pitchCos, ( rollSin * yawSin )  + ( yawCos * pitchSin * rollCos ), 0.f },
            { yawSin * pitchCos,                                       -pitchSin,            yawCos * pitchCos,                                     0.f },
            { 0.f,                                                     0.f,                0.f,                                                     1.f }
        } };
    }

    Matrix4x4 Matrix4x4::FromPositionAngles( const Vec3<f32> fvPosition, const Vec3Angle<f32> fvAngles )
    {
        const f32 yawCos = cosf( fvAngles.yaw );
        const f32 yawSin = sinf( fvAngles.yaw );
        const f32 pitchCos = cosf( fvAngles.pitch );
        const f32 pitchSin = sinf( fvAngles.pitch );
        const f32 rollCos = cosf( fvAngles.roll );
        const f32 rollSin = sinf( fvAngles.roll );

        return { .flMatrices = {
            { ( yawCos * rollCos ) + ( yawSin * pitchSin * rollSin ),  pitchCos * rollSin, ( -yawSin * rollCos ) + ( yawCos * pitchSin * rollSin ), 0.f },
            { ( -yawCos * rollSin ) + ( yawSin * pitchSin * rollCos ), rollCos * pitchCos, ( rollSin * yawSin )  + ( yawCos * pitchSin * rollCos ), 0.f },
            { yawSin * pitchCos,                                       -pitchSin,            yawCos * pitchCos,                                     0.f },
            { fvPosition.x,                                            fvPosition.y,         fvPosition.z,                                          1.f }
        } };
    }

    Matrix4x4 Matrix4x4::FromProjectionPerspective( const Vec2<uint> vnRenderSize, const float fldegFov, const float flNearDist, const float flFarDist )
    {
        const f32 flAspectRatio = f32( vnRenderSize.x ) / f32( vnRenderSize.y );
        const f32 flTanHalfFov = tanf( ( fldegFov * DEG_TO_RAD<f32> ) / 2.f );

        return { .flMatrices = {
            { 1.f / ( flAspectRatio * flTanHalfFov ), 0.f,                0.f,                                                      0.f  },
            { 0.f,                                    1.f / flTanHalfFov, 0.f,                                                      0.f  },
            { 0.f,                                    0.f,                flFarDist / ( flNearDist - flFarDist ),                   -1.f },
            { 0.f,                                    0.f,                -( flFarDist * flNearDist ) / ( flFarDist - flNearDist ), 0.f  }
        } };
    }

    Matrix4x4 Matrix4x4::MultiplyMatrix( const Matrix4x4 mat4First, const Matrix4x4 mat4Second )
    {
        // god this is ugly, lol
        Matrix4x4 r_matrix {};

        for ( uint i = 0; i < 4; i++ )
        {
            for ( uint j = 0; j < 4; j++ )
            {
                for ( uint k = 0; k < 4; k++ )
                {
                    r_matrix.flMatrices[ i ][ j ] += mat4First.flMatrices[ i ][ k ] * mat4Second.flMatrices[ k ][ j ];
                }
            }
        }

        return r_matrix;
    }
}

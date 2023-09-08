#pragma once

#include <libtitanium/util/maths.hpp>
#include <libtitanium/util/numerics.hpp>

#include <glm/glm.hpp>

namespace renderer
{
    constexpr int BINDGROUP_RENDERVIEW = 0;
    constexpr int BINDGROUP_RENDEROBJECT = 1;

    /*
     *  Builtin renderer uniforms
     *  TODO: need a way to define uniforms from code also
     */

    #pragma pack( push, 16 )
    struct UShaderView
    {
        // TODO: this should probably have the current time, but it doesn't seem necessary currently

        glm::mat4x4 m_mat4fCameraTransform;
        util::maths::Vec2<u32> m_vWindowSize;
        u64 pad;
    };
    #pragma pack( pop )
    static_assert( sizeof( UShaderView ) % 16 == 0 );

    #pragma pack( push, 16 )
    struct UShaderObjectInstance
    {
        glm::mat4x4 m_mat4fBaseTransform;
    };
    #pragma pack( pop )
    static_assert( sizeof( UShaderObjectInstance ) % 16 == 0 );
}

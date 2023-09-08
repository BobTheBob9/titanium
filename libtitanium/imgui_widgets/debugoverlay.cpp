#include "widgets.hpp"

#include <libtitanium/config/config.hpp>

namespace imguiwidgets
{
    config::Var<bool> * g_pbcvarRenderDebugOverlays = config::RegisterVar<bool>( "dev:debugoverlays", true, config::EFVarUsageFlags::NONE );

    bool BeginDebugOverlay()
    {
        if ( g_pbcvarRenderDebugOverlays->tValue )
        {
            ImGui::SetNextWindowPos( ImVec2( 0.f, 0.f ) );
            ImGui::Begin( "Debug Overlay", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize );

            return true;
        }
        else
        {
            return false;
        }
    }
}

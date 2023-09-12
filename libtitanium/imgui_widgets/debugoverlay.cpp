#include "widgets.hpp"

#include <libtitanium/config/config.hpp>

namespace imguiwidgets
{
    static bool s_bShowDebugOverlays = true;
    config::Var cvarShowDebugOverlays = config::RegisterVar( "dev::debugoverlays", config::EFVarUsageFlags::NONE, config::VARFUNCS_BOOL, &s_bShowDebugOverlays );

    bool BeginDebugOverlay()
    {
        if ( s_bShowDebugOverlays )
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

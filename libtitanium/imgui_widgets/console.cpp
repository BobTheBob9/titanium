#include "widgets.hpp"

#include "libtitanium/logger/logger.hpp"
#include "libtitanium/config/config.hpp"
#include "libtitanium/util/data/span.hpp"
#include "libtitanium/util/data/stringbuf.hpp"

config::Var<i32> * g_pncvarOutputSize =  config::RegisterVar<i32>( "console:outputsize", 60, config::EFVarUsageFlags::NONE );

namespace imguiwidgets
{
    // TODO: using void pointers for userdata sucks here, should be templated

    void Console( util::data::Span<char> spszConsoleInput, void * pCallbackUserData, util::data::StringBuf<128> ( *fnCommandHintCallback )( const util::data::Span<char> spszConsoleInput, void * pUserData ), void ( *fnCommandCompletionCallback )( const util::data::Span<char> spszConsoleInput, void * pUserData ) )
    {
        if ( ImGui::Begin( "Developer Console" ) )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f } );

            const ImVec2 vWindowSize = ImGui::GetWindowSize();

            ImGui::InputTextMultiline( "##ConsoleLog", "", 0, ImVec2( 0, vWindowSize.y - g_pncvarOutputSize->tValue ), ImGuiInputTextFlags_ReadOnly );

            if ( ImGui::InputText( "Input", spszConsoleInput.m_pData, spszConsoleInput.m_nElements, ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                fnCommandCompletionCallback( spszConsoleInput, pCallbackUserData );
                memset( spszConsoleInput.m_pData, '\0', spszConsoleInput.m_nElements );
            }

            ImGui::PopStyleVar();
        }
        ImGui::End();
    }
}

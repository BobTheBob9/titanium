#include "widgets.hpp"

#include "libtitanium/logger/logger.hpp"

namespace imguiwidgets
{
    // StringBuf<128>( *fnCommandHintCallback )(  const util::data::Span<char> spszConsoleInput )
    void Console( util::data::Span<char> spszConsoleInput, void ( *fnCommandCompletionCallback )( const util::data::Span<char> spszConsoleInput ) )
    {
        if ( ImGui::Begin( "Developer Console" ) )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f } );

            const ImVec2 vWindowSize = ImGui::GetWindowSize();

            ImGui::InputTextMultiline( "##ConsoleLog", "", 0, ImVec2( 0, vWindowSize.y - 60 ), ImGuiInputTextFlags_ReadOnly );

            if ( ImGui::InputText( "Input", spszConsoleInput.m_pData, spszConsoleInput.m_nElements, ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                fnCommandCompletionCallback( spszConsoleInput );
                memset( spszConsoleInput.m_pData, '\0', spszConsoleInput.m_nElements );
            }

            ImGui::PopStyleVar();
        }
        ImGui::End();
    }
}

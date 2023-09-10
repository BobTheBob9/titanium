#include "imgui.h"
#include "widgets.hpp"

#include <libtitanium/config/config.hpp>
#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/data/staticspan.hpp>
#include <libtitanium/util/data/stringbuf.hpp>

namespace imguiwidgets
{
    // TODO: using void pointers for userdata sucks here, should be templated

    struct C_ConsoleUserData
    {
        char * pszSelectedEntry;
        int nSelectionChange;
    };

    int C_ConsoleInput( ImGuiInputTextCallbackData *const pimguiCallbackData )
    {
        C_ConsoleUserData * pCallbackUserData = static_cast<C_ConsoleUserData *>( pimguiCallbackData->UserData );
        if ( pimguiCallbackData->EventFlag == ImGuiInputTextFlags_CallbackCompletion )
        {
            pimguiCallbackData->BufDirty = true;
            strcpy( pimguiCallbackData->Buf, pCallbackUserData->pszSelectedEntry );
            pimguiCallbackData->BufTextLen = strlen( pimguiCallbackData->Buf );
            pimguiCallbackData->CursorPos = pimguiCallbackData->BufTextLen;
        }
        else if ( pimguiCallbackData->EventFlag == ImGuiInputTextFlags_CallbackHistory && ImGui::GetIO().KeyShift )
        {
            pCallbackUserData->nSelectionChange = pimguiCallbackData->EventKey == ImGuiKey_DownArrow ? -1 : 1;
        }

        return 0;
    }

    void Console( util::data::Span<char> spszConsoleInput, void * pCallbackUserData,
                  void ( *fnCommandHintCallback )( const util::data::Span<char> spszConsoleInput, const util::data::Span<util::data::StringBuf<128>> o_spszAutocompleteItems, void * pCallbackUserData ),
                  void ( *fnCommandCompletionCallback )( const util::data::Span<char> spszConsoleInput, void * pUserData ) )
    {
        if ( ImGui::Begin( "Developer Console" ) )
        {
            util::data::StaticSpan<util::data::StringBuf<128>, 10> spszAutocompleteItems;
            fnCommandHintCallback( spszConsoleInput, spszAutocompleteItems.ToConstSpan(), pCallbackUserData );

            // TODO: do this in a dropdown
            ImGui::InputTextMultiline( "##ConsoleLog", util::data::StringBuf<1>().ToCStr(), 0, ImVec2( 0, -( ImGui::GetTextLineHeightWithSpacing() * 1.1 ) ), ImGuiInputTextFlags_ReadOnly );

            // TODO: do this in callback
            C_ConsoleUserData callbackUserData { .pszSelectedEntry = spszAutocompleteItems.m_tData[ 0 ].ToCStr() };
            if ( ImGui::InputText( "Input", spszConsoleInput.m_pData, spszConsoleInput.m_nElements, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, C_ConsoleInput, &callbackUserData ) )
            {
                fnCommandCompletionCallback( spszConsoleInput, pCallbackUserData );
                memset( spszConsoleInput.m_pData, '\0', spszConsoleInput.m_nElements );
            }

            ImVec2 vAutocompletePos = ImGui::GetItemRectMin();
            vAutocompletePos.y += ImGui::GetItemRectSize().y;
            ImGui::End();

            ImGui::SetNextWindowPos( vAutocompletePos );
            ImGui::Begin( "ConsoleAutocomplete", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoSavedSettings );
            {
                for ( int i = 0; i < spszAutocompleteItems.Elements() && *spszAutocompleteItems.m_tData[ i ].m_szStr; i++ )
                {
                    ImGui::PushID( i );
                    if ( ImGui::Selectable( spszAutocompleteItems.m_tData[ i ].ToCStr(), false ) )
                    {
                        strcpy( spszConsoleInput.m_pData, spszAutocompleteItems.m_tData[ 0 ].ToCStr() );
                    }
                    ImGui::PopID();
                }
            }
            ImGui::End();
        }
    }
}

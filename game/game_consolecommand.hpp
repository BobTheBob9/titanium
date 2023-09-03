#pragma once

#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/data/stringbuf.hpp>

/*
 *  Send a command to the engine, intended for use with the graphical console or remote server management software
 *  While this *can* be called at any point, I'd recommend only calling it from a single point in the frameloop, just to avoid complicating things
 */
void SubmitCommand( const util::data::Span<char> spszConsoleInput );

/*
 *  For use with the imgui console widget
 */
void C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, const util::data::Span<util::data::StringBuf<128>> o_spszAutocompleteItems, void * pCallbackUserData );
void C_ConsoleCommandCompletion( const util::data::Span<char> spszConsoleInput, void * pCallbackUserData );

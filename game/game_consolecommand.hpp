#pragma once

#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/data/stringbuf.hpp>

void SubmitCommand( const util::data::Span<char> spszConsoleInput );

void C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, const util::data::Span<util::data::StringBuf<128>> o_spszAutocompleteItems, void * pCallbackUserData );
void C_ConsoleCommandCompletion( const util::data::Span<char> spszConsoleInput, void * pCallbackUserData );

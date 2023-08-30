#pragma once

#include <config/config.hpp>

#include <libtitanium/util/data/span.hpp>
#include <libtitanium/util/data/stringbuf.hpp>

util::data::StringBuf<128>  C_ConsoleAutocomplete( const util::data::Span<char> spszConsoleInput, void * pUserData );
void C_ConsoleCommandCompletion( const util::data::Span<char> spszConsoleInput, void * pCallbackUserData );

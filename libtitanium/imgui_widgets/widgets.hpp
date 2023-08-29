#pragma once

#include "extern/imgui/imgui.h"

#include "libtitanium/util/data/span.hpp"
#include "libtitanium/util/data/stringbuf.hpp"

namespace imguiwidgets
{
    void Console( util::data::Span<char> spszConsoleInput, void * pCallbackUserData, util::data::StringBuf<128> ( *fnCommandHintCallback )( const util::data::Span<char> spszConsoleInput, void * pUserData ), void ( *fnCommandCompletionCallback )( const util::data::Span<char> spszConsoleInput, void * pUserData ) );
    void ResourceProfiler();
}

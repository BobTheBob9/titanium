#pragma once

#include "extern/imgui/imgui.h"

#include "libtitanium/util/data/span.hpp"

namespace imguiwidgets
{
    void Console( util::data::Span<char> spszConsoleInput, void ( *fnCommandCompletionCallback )( const util::data::Span<char> spszConsoleInput ) );
    void ResourceProfiler();
}

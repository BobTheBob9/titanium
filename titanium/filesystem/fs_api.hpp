#pragma once

#include <filesystem> // for std::fs::path atm

// TODO: should we have a custom filepath class/impl?
namespace std { namespace fs = std::filesystem; } // alias std::filesystem => std::fs

#ifndef FS_PAK_SUPPORT
    #define FS_PAK_SUPPORT 0
#endif // #ifndef FS_PAK_SUPPORT

namespace filesystem
{
    void Initialise( const std::fs::path fDirectory );

    void CreateDirectories();
    void Open();
};

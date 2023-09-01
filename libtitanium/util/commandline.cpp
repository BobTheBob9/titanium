#include "commandline.hpp"

#include <libtitanium/util/data/staticspan.hpp>
#include <libtitanium/util/string.hpp>
#include <libtitanium/memory/mem_core.hpp>
#include <libtitanium/dev/tests.hpp>

#include <string.h>

namespace util::commandline
{
    void CreateFromSystemWithAlloc( CommandArgs *const pcaCommandArgs, const int nArgs, const char *const *const ppszArgs )
    {
        // TODO: if we get a proper string copy function that takes the null terminator also, use this here
        // also
        // TODO update for new mem alloc stuff

        // Figure out the size of the buffer in pcaCommandArgs.m_sBuffer
        size_t nBufSize = 0;
        for ( int i = 0; i < nArgs; i++ )
        {
            nBufSize += util::string::LengthOfCStringWithTerminator( ppszArgs[ i ] );
        }

        // create copy strings to the argument buffer
        size_t nBufIndex = 0;
        char *const pArgBuffer = memory::alloc_nT<char>( nBufSize );
        for ( int i = 0; i < nArgs; i++ )
        {
            size_t nCopyAmount = util::string::LengthOfCStringWithTerminator( ppszArgs[ i ] );
            memcpy( pArgBuffer + nBufIndex, ppszArgs[ i ], nCopyAmount );

            nBufIndex += nCopyAmount;
        }

        // copy to out command args struct
        pcaCommandArgs->m_eBufferSource = CommandArgs::EBufferSource::SYSTEM_COPY;
        pcaCommandArgs->m_nArgumentStrings = nArgs;
        pcaCommandArgs->m_sBuffer = util::data::Span<char>( nBufSize, pArgBuffer );
    }

    /*
     *  For when the buffer you're passing to this function is created by you, and so you control its lifetime
     *  CommandArgs structs created from this function allocate no memory, instead just using the pointer passed to this function as their buffer
     *  This function destructively replaces separators between arguments with null pointers, use CreateFromBufferWithAlloc to create CommandArgs non-destructively, with a memory allocation
     */
    void CreateFromBufferDestructive( CommandArgs *const pcaCommandArgs )
    {

    }

    void CreateFromBufferWithAlloc( CommandArgs *const pcaCommandArgs )
    {

    }

    /*
     *  Gets the next argument given the current argument in the argument buffer, if no arguments are left, returns nullptr
     *  if pszCurrentArgument is nullptr, returns the first argument in the buffer
     */
    const char * GetNextArgument( CommandArgs *const pcaCommandArgs, const char *const pszCurrentArgument )
    {
        if ( !pszCurrentArgument ) [[ unlikely ]]
        {
            return pcaCommandArgs->m_sBuffer.m_pData;
        }

        const char *const pBufUpperIndex = pcaCommandArgs->m_sBuffer.m_pData + pcaCommandArgs->m_sBuffer.m_nElements;
        assert::Debug( util::maths::WithinRange<const void *>( pszCurrentArgument, pcaCommandArgs->m_sBuffer.m_pData, pBufUpperIndex ) );

        // iterate from the current arg, to the next, then return it
        // if we exceed the argument buffer, return nullptr
        for ( const char * pszCharIterator = pszCurrentArgument + 1; pszCharIterator < pBufUpperIndex; pszCharIterator++ )
        {
            if ( !*( pszCharIterator - 1 ) ) 
            {
                return pszCharIterator;
            }
        }

        return nullptr;
    }

    /*
     *  Finds the value of the argument passed into the function, i.e. the string immediately after it
     */
    const char * GetArgumentValue( CommandArgs *const pcaCommandArgs, const char *const pszArgumentToFind ) 
    { 
        // find the position of the argument passed in
        const char * pszArgIterator = nullptr;
        while ( ( pszArgIterator = GetNextArgument( pcaCommandArgs, pszArgIterator ) ) )
        {
            if ( !strcmp( pszArgIterator, pszArgumentToFind ) )
            {
                break;
            }
        }

        if ( !pszArgIterator )
        {
            return nullptr;
        }

        // return the value of the argument (string after the passed in argument), or nullptr if buffer is over
        return GetNextArgument( pcaCommandArgs, pszArgIterator );
    }

    /*
     *  
     */
    R_FindArgumentPair GetNextArgumentPairByWildcard( CommandArgs *const pcaCommandArgs, const char *const pszSearchString, const size_t nSearchStringLength, const char *const pszCurrentArgument )
    {
        const char * pszArgIterator = pszCurrentArgument;
        while ( ( pszArgIterator = GetNextArgument( pcaCommandArgs, pszArgIterator ) ) )
        {
            if ( !strncmp( pszArgIterator, pszSearchString, nSearchStringLength ) )
            {
                R_FindArgumentPair r_pair;
                r_pair.pszKey = pszArgIterator;
                r_pair.pszValue = GetNextArgument( pcaCommandArgs, pszArgIterator );
                r_pair.bFound = r_pair.pszValue != nullptr;

                return r_pair;
            }
        }

        return { .bFound = false };
    }

    bool HasArgument( CommandArgs *const pcaCommandArgs, const char *const pszArgumentToFind )
    {
        const char * pszArgIterator = nullptr;
        while ( ( pszArgIterator = GetNextArgument( pcaCommandArgs, pszArgIterator ) ) )
        {
            if ( util::string::CStringsEqual( pszArgIterator, pszArgumentToFind ) )
            {
                return true;
            }
        }

        return false;
    }

    void Free( CommandArgs *const pcaCommandArgs )
    {
        if ( pcaCommandArgs->m_eBufferSource < CommandArgs::EBufferSource::__MAX_COPY )
        {
            memory::free( pcaCommandArgs->m_sBuffer.m_pData );
        }
    }
}

#if USE_TESTS
    TEST( Commandline )
    {
        util::commandline::CommandArgs caArgs;
        util::data::StaticSpan<const char *, 9> sCommandlineArgs { 
            "hi", 
            "+connect", "localhost", 
            "wow", 
            "+sv_cheats", "1", 
            "+mp_enablematchending", "5", 
            "cool" 
        };
        
        util::commandline::CreateFromSystemWithAlloc( &caArgs, sCommandlineArgs.Elements(), sCommandlineArgs.m_tData );
        
        TEST_EXPECT( caArgs.m_nArgumentStrings == sCommandlineArgs.Elements() );
        TEST_EXPECT( caArgs.m_eBufferSource == util::commandline::CommandArgs::EBufferSource::SYSTEM_COPY );

        {
            const char * pszArgIterator = nullptr;
            int i = 0;
            while ( ( pszArgIterator = GetNextArgument( &caArgs, pszArgIterator ) ) )
            {
                TEST_EXPECT( !strcmp( pszArgIterator, sCommandlineArgs.m_tData[ i++ ] ) );
            }
        }

        TEST_EXPECT( !strcmp( util::commandline::GetArgumentValue( &caArgs, "+sv_cheats" ), "1" ) );
        TEST_EXPECT( !strcmp( util::commandline::GetArgumentValue( &caArgs, "+connect" ), "localhost" ) );

        {
            // find the values of all arguments prefixed with "+", e.g. for finding convar values
            util::commandline::R_FindArgumentPair argIterator {};
            while ( ( argIterator = util::commandline::GetNextArgumentPairByWildcard( &caArgs, "+", 1, argIterator.pszValue ) ).bFound )
            {
                TEST_EXPECT( !strcmp( util::commandline::GetArgumentValue( &caArgs, argIterator.pszKey ), argIterator.pszValue ) );
            }
        }

        TEST_EXPECT( util::commandline::GetArgumentValue( &caArgs, "cool" ) == nullptr );

        return true;
    }
#endif // #if USE_TESTS

#include "string.hpp"

#include "titanium/memory/mem_core.hpp"
#include "titanium/dev/tests.hpp"
#include "titanium/logger/logger.hpp"

#include "extern/simde-no-tests/x86/sse.h"
#include "extern/simde-no-tests/x86/sse2.h"

#include <string.h>

namespace util::string
{
    char * AllocForSIMD( const size_t nChars )
    {
        // the + and - 1s here are to force rounding up (cxx diving truncates towards zero, so rounds down with unsigneds), so trying to allocate 18 chars actually allocates 32
        constexpr int ALIGNMENT = 16;
        size_t nSize = ALIGNMENT * ( nChars - 1 / ALIGNMENT + 1 );

        char * pResult = memory::alloc_nT<char>( nSize );
        memset( pResult, 0, nSize );

        return pResult;
    }

    int Length( char *const pszStringToCheck )
    {
        int nChars = 0;
        for ( char * pszStringIterator = pszStringToCheck; *pszStringIterator; pszStringIterator++ )
        {
            nChars++;
        }

        return nChars;
    }
    
    int Length_SSE( char *const pa16szStringToCheck )
    {
        int nChars = 0;
        for ( char * pszStringIterator = pa16szStringToCheck; *pszStringIterator; pszStringIterator += 16 )
        {
            const simde__m128i simdCurrentChars = simde_mm_load_si128( ( simde__m128i * )pszStringIterator );
            const simde__m128i simdIsNullchar = simde_mm_cmpeq_epi8( simdCurrentChars, simde_mm_setzero_si128() );

            const u16 nBitmask = simde_mm_movemask_epi8( simdIsNullchar ); // get a bitmask out of it, where each set bit == nonzero char
            // TODO: BitScanForward or equivalent here, and remove std dependency
            //nChars += std::popcount<u16>( ~nBitmask ); // count number of nonzero bits
            nChars += __builtin_popcount( ~nBitmask ) - 16; // __builtin_popcount takes an int32, so -16 bytes that will always be 0

            if ( nBitmask )
                break;
        }

        return nChars;
    }

    void CopyTo( const char *const pszSource, char *const pszDestinationBuffer )
    {
        int i;
        for ( i = 0; pszSource[ i ]; i++ )
        {
            pszDestinationBuffer[ i ] = pszSource[ i ];
        }

        pszDestinationBuffer[ i ] = '\0';
    }

    void CopyTo_SSE( const char *const pa16szSource, char *const pa16szDestinationBuffer )
    {
        // NOTE: we don't have any kind of condition in this loop as we should only break after we've copied the nullchar
        for ( int i = 0;; i += 16 )
        {
            const simde__m128i simdCurrentChars = simde_mm_load_si128( ( simde__m128i * )( pa16szSource + i ) );
            simde_mm_storeu_si128( ( simde__m128i * )( pa16szDestinationBuffer + i ), simdCurrentChars ); // copy to buffer

            // check for nullchars, break if any are found
            const simde__m128i simdIsNullchar = simde_mm_cmpeq_epi8( simdCurrentChars, simde_mm_setzero_si128() );
            if ( simde_mm_movemask_epi8( simdIsNullchar ) ) // check if mask is nonzero
                break;
        }
    } 

    void CopyToWithBufferSize( const char *const pszSource, char *const pszDestinationBuffer, const size_t nDestinationBufferSize )
    {
        for ( int i = 0; pszSource[ i ] && i < nDestinationBufferSize; i++ )
        {
            pszDestinationBuffer[ i ] = pszSource[ i ];
        }

        pszDestinationBuffer[ nDestinationBufferSize - 1 ] = '\0';
    }

    // TODO: CopyToWithBufferSize_SSE doesn't really make sense i think? we can't really block copy if the buffer size isn't necessarily 16 byte aligned
    // unsure, i suppose maybe we could just exit if remaining buf < 16 chars

    // NOTE: these work because all lowercase ascii chars are +32 from uppercase
    void ToLowercase( char *const pszStringToConvert )
    {
        for ( char * pszStringIterator = pszStringToConvert; *pszStringIterator; pszStringIterator++ )
        {
            *pszStringIterator += 32 * ( *pszStringIterator >= 'A' && *pszStringIterator <= 'Z' );
        }
    }

    void ToLowercase_SSE( char *const pa16szStringToConvert )
    {
        // TODO: could we make this a compiletime constant? unsure
        const simde__m128i simdAsciiA = simde_mm_set1_epi8( 'A' );
        const simde__m128i simdAsciiZ = simde_mm_set1_epi8( 'Z' + 1 ); // + 1 for inclusive range in less than check later

        const simde__m128i simdLowercaseConversionDiff = simde_mm_set1_epi8( 32 ); // diff between lowercase and uppercase ascii chars

        // NOTE: we check if *pszStringIterator == nullchar each loop, HOWEVER
        //       this is only for if the first char of the iteration == nullchar, we do a manual check AFTER each iteration for nullchar
        for ( char * pszStringIterator = pa16szStringToConvert; *pszStringIterator; pszStringIterator += 16 )
        {
            const simde__m128i simdCurrentChars = simde_mm_load_si128( ( const simde__m128i * )pszStringIterator );

            // check if chars are within uppercase range
            // each byte of these comparisons are set to 0xFF if true, or 0x0 if false, this is important for bitmasker-y later
            const simde__m128i simdGreaterThanA = simde_mm_cmpgt_epi8( simdCurrentChars, simdAsciiA );
            const simde__m128i simdLessThanZ = simde_mm_cmplt_epi8( simdCurrentChars, simdAsciiZ );

            // create a bitmask by &-ing together the results of the greater than and less than comparisons
            // the results of the previous comparisons are either true == 0xFF or false == 0x0, &-ing together these will only result in true if both are true
            // i.e. if char > A and char < Z!
            const simde__m128i simdValidCharsMask = simde_mm_and_si128( simdGreaterThanA, simdLessThanZ );

            // make a mask to add values to our vector
            // we & our difference (32) with our previous comparison, so we add 32 to all valid chars, and 0 to all invalid ones
            // then add to the char values!
            const simde__m128i simdAddMask = simde_mm_and_si128( simdValidCharsMask, simdLowercaseConversionDiff );
            const simde__m128i simdConvertedChars = simde_mm_add_epi8( simdCurrentChars, simdAddMask );

            // store the result back in the string we're modifying
            simde_mm_storeu_si128( ( simde__m128i * )pszStringIterator, simdConvertedChars );

            // finally, we need to check if there are any nullchars in the current chunk, so we can exit
            const simde__m128i simdIsNullchar = simde_mm_cmpeq_epi8( simdCurrentChars, simde_mm_setzero_si128() );
            if (  simde_mm_movemask_epi8( simdIsNullchar ) ) // check if mask is nonzero
                break;
        }
    }

    void ToUppercase( char *const pszStringToConvert )
    {
        for ( char * pszStringIterator = pszStringToConvert; *pszStringIterator; pszStringIterator++ )
        {
            *pszStringIterator -= 32 * ( *pszStringIterator >= 'a' && *pszStringIterator <= 'z' );
        }
    }

    void ToUppercase_SSE( char *const pa16szStringToConvert )
    {
        // see ToLowercase_SSE for comments/explanation
        const simde__m128i simdAsciia = simde_mm_set1_epi8( 'a' );
        const simde__m128i simdAsciiz = simde_mm_set1_epi8( 'z' + 1 );

        const simde__m128i simdUppercaseConversionDiff = simde_mm_set1_epi8( 32 );

        for ( char * pszStringIterator = pa16szStringToConvert; *pszStringIterator; pszStringIterator += 16 )
        {
            const simde__m128i simdCurrentChars = simde_mm_load_si128( ( simde__m128i * )pszStringIterator );

            const simde__m128i simdGreaterThana = simde_mm_cmpgt_epi8( simdCurrentChars, simdAsciia );
            const simde__m128i simdLessThanz = simde_mm_cmplt_epi8( simdCurrentChars, simdAsciiz );

            const simde__m128i simdValidCharsMask = simde_mm_and_si128( simdGreaterThana, simdLessThanz );

            const simde__m128i simdSubtractMask = simde_mm_and_si128( simdValidCharsMask, simdUppercaseConversionDiff );
            const simde__m128i simdConvertedChars = simde_mm_sub_epi8( simdCurrentChars, simdSubtractMask );

            simde_mm_storeu_si128( ( simde__m128i * )pszStringIterator, simdConvertedChars );

            const simde__m128i simdIsNullchar = simde_mm_cmpeq_epi8( simdCurrentChars, simde_mm_setzero_si128() );
            if ( simde_mm_movemask_epi8( simdIsNullchar ) )
                break;
        }
    }
};

#if USE_TESTS
    TEST( SSEString )
    {
        constexpr const char *const UPPERCASE_TEXT = "WOWWWWWWWWWW!!!!WWWWWWWWWWWWWWWW";

        char * pszStringForNonSSE = memory::alloc_nT<char>( strlen( UPPERCASE_TEXT ) + 1 );
        char * pszStringForSSE = util::string::AllocForSIMD( strlen( UPPERCASE_TEXT ) + 1 );

        util::string::CopyTo( UPPERCASE_TEXT, pszStringForNonSSE );
        util::string::CopyTo_SSE( pszStringForNonSSE, pszStringForSSE );

        TEST_EXPECT( !strcmp( pszStringForNonSSE, pszStringForSSE ) );

        util::string::ToLowercase( pszStringForNonSSE );
        util::string::ToLowercase_SSE( pszStringForSSE );

        TEST_EXPECT( !strcmp( pszStringForNonSSE, pszStringForSSE ) );

        util::string::ToUppercase( pszStringForNonSSE );
        util::string::ToUppercase_SSE( pszStringForSSE );

        TEST_EXPECT( !strcmp( pszStringForNonSSE, pszStringForSSE ) );
        TEST_EXPECT( !strcmp( pszStringForSSE, UPPERCASE_TEXT ) );

        TEST_EXPECT( util::string::Length( pszStringForNonSSE ) == strlen( UPPERCASE_TEXT ) );
        TEST_EXPECT( util::string::Length_SSE( pszStringForSSE ) == strlen( UPPERCASE_TEXT ) );

        return true;
    }
#endif // #if USE_TESTS
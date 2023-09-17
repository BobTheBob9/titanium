#include "tests.hpp"

#include <libtitanium/util/data/vector.hpp>
#include <libtitanium/logger/logger.hpp>

#if USE_TESTS
namespace dev::tests
{
    struct TestEntry
    {
        const char * m_pszTestName;
        FnTest m_fnTest;

        TestEntry( const char* pszTestName, const FnTest fnTest ) 
        { 
            m_pszTestName = pszTestName;
            m_fnTest = fnTest;
        }
    };

    static util::data::Vector<TestEntry> s_vTests;

    void AddTest( const char *const pszTestName, const FnTest fnTest )
    {
        s_vTests.AppendWithAlloc( TestEntry( pszTestName, fnTest ) );
    }

    bool RunTests()
    {
        for ( uint i = 0; i < s_vTests.Length(); i++ )
        {
            logger::Info( "Running test %s" ENDL, s_vTests.GetAt( i )->m_pszTestName );

            if ( !s_vTests.GetAt( i )->m_fnTest() )
            {
                return false;
            }
        }

        return true;
    }
}
#endif // #if USE_TESTS

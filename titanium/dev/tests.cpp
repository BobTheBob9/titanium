#include "tests.hpp"

#include "titanium/util/data/vector.hpp"
#include "titanium/logger/logger.hpp"

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

    void RunTests()
    {
        for ( int i = 0; i < s_vTests.Length(); i++ )
        {
            logger::Info( "Running test %s" ENDL, s_vTests.GetAt( i )->m_pszTestName );
            s_vTests.GetAt( i )->m_fnTest();
        }
    }
}
#endif // #if USE_TESTS
#pragma once

#include <libtitanium/util/staticinitialise.hpp>
#include <libtitanium/logger/logger.hpp>

/*

tests contains various functions for defining and running tests on code

*/
namespace dev::tests
{
    /*
    
    USE_TESTS controls whether we should compile in test code
    
    */
    #ifndef USE_TESTS
        #define USE_TESTS 1
    #endif  // #ifndef USE_TESTS

#if USE_TESTS
    using FnTest = bool(*)();
    void AddTest( const char *const pszTestName, const FnTest fnTest );
    bool RunTests();

    // Macro that defines a named test function and adds it to the global tests object
    #define TEST( name ) bool __##name##Test(); namespace { static util::StaticInitialise __##name##TestINITIALISER( [](){ dev::tests::AddTest( #name, __##name##Test ); } ); }; bool __##name##Test()
    #define TEST_EXPECT( cond )  \
        logger::Info( "Test \"%s\" ", #cond ); \
        if ( !( cond ) ) \
        { \
            logger::Info( "failed" ENDL ); \
            return false; \
        } else logger::Info( "passed" ENDL ); \

#endif // #if USE_TESTS
}

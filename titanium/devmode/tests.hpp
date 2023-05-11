#pragma once

#include "titanium/util/staticinitialise.hpp"
#include "titanium/util/data/vector.hpp"

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
    using TestFunc = void(*)();

    struct TestEntry
    {
        const char * m_pszTestName;
        TestFunc m_fnTestFunc;

        TestEntry( const char* pszTestName, TestFunc fnTestFunc ) 
        { 
            m_pszTestName = pszTestName;
            m_fnTestFunc = fnTestFunc;
        }
    };

    // this is a function that returns a static property so we don't have any ***fun*** with static initialiser orderings
    inline utils::data::Vector<TestEntry>* AllTests() { static utils::data::Vector<TestEntry> s_pvTests; return &s_pvTests; }

    // Macro that defines a named test function and adds it to the global tests object
    #define TEST( name ) void __##name##Test(); namespace { static utils::StaticInitialise __##name##TestINITIALISER( [](){ dev::tests::AllTests()->AppendWithAlloc( dev::tests::TestEntry( #name, __##name##Test ) ); } ); }; void __##name##Test()
#else // #if USE_TESTS
    #define TEST( name ) void __##name##TestUNUSED()
#endif // #if USE_TESTS
}

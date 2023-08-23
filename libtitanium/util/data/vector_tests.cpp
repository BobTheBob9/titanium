#if USE_TESTS
    TEST( Vector )
    {
        util::data::Vector<u64> vnTestVec;
    
        TEST_EXPECT( vnTestVec.Length() == 0 );
    
        constexpr int GARBAGE_LENGTH = 299;
        u64 pnGarbageData[ GARBAGE_LENGTH ];
        LOG_CALL( vnTestVec.AppendMultipleWithAlloc( util::data::Span<u64>( GARBAGE_LENGTH, pnGarbageData ) ) );
        TEST_EXPECT( vnTestVec.Length() == GARBAGE_LENGTH );
    
        vnTestVec.AppendWithAlloc( 99 );
        TEST_EXPECT( vnTestVec.Length() == GARBAGE_LENGTH + 1 );
        TEST_EXPECT( vnTestVec.ElementsAllocated() >= GARBAGE_LENGTH + ( GARBAGE_LENGTH / 2 ) );
    
        const util::data::Vector<u64>::R_IndexOf_s r_indexOf = vnTestVec.IndexOf( 99 );
        TEST_EXPECT( r_indexOf.bFound );
        TEST_EXPECT( vnTestVec.Contains( 99 ) );
        TEST_EXPECT( vnTestVec.GetAt( r_indexOf.nIndex )  );
        TEST_EXPECT( *vnTestVec.GetAt( r_indexOf.nIndex ) == 99 );
            
        while ( vnTestVec.Length() )
        {
            const u64 *const pnLastValue = vnTestVec.GetAt( vnTestVec.LastIndex() );
            vnTestVec.Remove( *pnLastValue );
        }
    
        TEST_EXPECT( vnTestVec.Length() == 0 );
        TEST_EXPECT( vnTestVec.ElementsAllocated() == 0 );
    
        return true;
    }
#endif // #if USE_TESTS

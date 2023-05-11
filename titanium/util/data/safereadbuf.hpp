#pragma once

#include <concepts>
#include <type_traits>

#include "titanium/util/numerics.hpp"
#include "titanium/util/contract.hpp"
#include "titanium/util/assert.hpp"
#include "titanium/util/data/span.hpp"

namespace utils::data
{
    template <typename T>
    concept ISafeReadBuf = requires(T t)
    {
        { &T::Size } -> std::same_as< u32 ( T::* )() const >;

        { &T::SetPosition } -> std::same_as< void ( T::* )( const u32 nPosition ) >;

        { &T::ReadByte } -> std::same_as< byte ( T::* )() >;
        { &T::ReadU8 } -> std::same_as< u8 ( T::* )() >;
        { &T::ReadU16 } -> std::same_as< u16 ( T::* )() >;
        { &T::ReadU32 } -> std::same_as< u32 ( T::* )() >;
        { &T::ReadU64 } -> std::same_as< u64 ( T::* )() >;

        { &T::ReadBytes } -> std::same_as< const byte * ( T::* )( const u32 nLength ) >;
    };
    
    template <std::unsigned_integral TSize = u32> 
    class SafeReadBufBasic
    {
        TSize m_nPosition = 0;
        const utils::data::Span<byte, TSize> m_Data;

    public:
        SafeReadBufBasic() = delete;
        SafeReadBufBasic( const Span<byte, TSize> data );
        SafeReadBufBasic( const TSize nElements, const byte *const pData );

        TSize Size() const;

        void SetPosition( const TSize nPosition ); 
        
        byte ReadByte();
        u8 ReadU8();
        u16 ReadU16();
        u32 ReadU32();
        u64 ReadU64();
        const byte* ReadBytes( const TSize nBytes );
    };


    // ////////////// //
    // IMPLEMENTATION //
    // ////////////// //

    template <std::unsigned_integral TSize> 
    SafeReadBufBasic<TSize>::SafeReadBufBasic( const Span<byte, TSize> data ) : m_Data( data ) {}
    template <std::unsigned_integral TSize> 
    SafeReadBufBasic<TSize>::SafeReadBufBasic( const TSize nElements, const byte *const pData ) : m_Data( utils::data::Span<byte, TSize>( nElements, pData ) ) {}

    template <std::unsigned_integral TSize> 
    TSize SafeReadBufBasic<TSize>::Size() const
    {
        return m_Data.m_nElements;
    }


    template <std::unsigned_integral TSize> 
    void SafeReadBufBasic<TSize>::SetPosition( const TSize nPosition )
    {
        assert::Debug( nPosition < Size() );
        m_nPosition = nPosition;
    }


    template <std::unsigned_integral TSize> 
    byte SafeReadBufBasic<TSize>::ReadByte()
    {
        assert::Release( m_nPosition + sizeof( byte ) < Size() );

        byte value = m_Data.m_pData[ m_nPosition ];
        m_nPosition += sizeof( byte );

        return value;
    }

    template <std::unsigned_integral TSize> 
    u16 SafeReadBufBasic<TSize>::ReadU16()
    {
        assert::Release( m_nPosition + sizeof( u16 ) < Size() );

        u16 value = m_Data.m_pData[ m_nPosition ];
        m_nPosition += sizeof( u16 );

        return value;
    }

    template <std::unsigned_integral TSize> 
    u32 SafeReadBufBasic<TSize>::ReadU32()
    {
        assert::Release( m_nPosition + sizeof( u32 ) < Size() );

        u32 value = m_Data.m_pData[ m_nPosition ];
        m_nPosition += sizeof( u32 );

        return value;
    }

    template <std::unsigned_integral TSize> 
    u64 SafeReadBufBasic<TSize>::ReadU64()
    {
        assert::Release( m_nPosition + sizeof( u64 ) < Size() );

        u64 value = m_Data.m_pData[ m_nPosition ];
        m_nPosition += sizeof( u64 );

        return value;
    }

    template <std::unsigned_integral TSize> 
    const byte* SafeReadBufBasic<TSize>::ReadBytes( const TSize nBytes )
    {
        assert::Release( m_nPosition + nBytes * sizeof( byte ) < Size() );
        
        const byte* data = m_Data.m_pData + m_nPosition;
        m_nPosition += m_nPosition;

        return data;
    }
};

#include <type_traits>

#define ENUM_FLAGS_DECLARE( T ) T operator&( const T tFirst, const T tSecond ); T operator|( const T tFirst, const T tSecond )

#define ENUM_FLAGS( T ) \
T operator&( const T tFirst, const T tSecond )                                                              \
{                                                                                                           \
    return T( std::underlying_type<T>::type( tFirst ) & std::underlying_type<T>::type( tSecond ) );         \
}                                                                                                           \
                                                                                                            \
T operator|( const T tFirst, const T tSecond )                                                              \
{                                                                                                           \
    return T( std::underlying_type<T>::type( tFirst ) & std::underlying_type<T>::type( tSecond ) );         \
}                                                                                                           \


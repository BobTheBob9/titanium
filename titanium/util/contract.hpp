#pragma once

#include <concepts> 

#define CONTRACT( name ) template<typename __contract_T> concept name = requires( __contract_T __contract_T_instance )
#define CONTRACT_STATIC( staticMemberName, type ) std::is_same< decltype( __contract_T::staticMemberName ), type >::value
#define CONTRACT_MEMBER( memberName, type ) std::is_same< decltype( __contract_T_instance->memberName ), type >::value

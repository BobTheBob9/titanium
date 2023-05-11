// devmode/tests.hpp includes util/data/vector.hpp, so we can't really include tests.hpp in vector.hpp
// otherwise we get various uncompileable weirdness! so need a new file

#include "vector.hpp"   

#include "titanium/devmode/tests.hpp"
#include "titanium/util/assert.hpp"

TEST( Vector )
{
    utils::data::Vector<u32> vnTestVec;
    //#error "oops no tests"!
}
#include "Lib/containers/StaticArray.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class StaticArray<int, 1>;
template class StaticArray<float, 2>;
template class StaticArray<String, 3>;
template class StaticArray<String*, 4>;
template class StaticArray<NoncopyableMovable, 5>;

// TODO: this throws MSVC warning 4623: default constructor was implicitly defined as deleted
// struct NonDefaultConstructible {
//    NonDefaultConstructible() = delete;
//    NonDefaultConstructible(const NonDefaultConstructible&);
//};
// template class StaticArray<NonDefaultConstructible, 5>;

TEST_CASE("StaticArray construct") {
    [[maybe_unused]] constexpr StaticArray<int, 5> ARRAY = {0, 1, 2, 3, 4};
}

BUFF_NAMESPACE_END

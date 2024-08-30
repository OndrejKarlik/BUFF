#include "Lib/containers/ArrayView.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/containers/Array.h"
#include "Lib/Pixel.h"

BUFF_NAMESPACE_BEGIN

// Test compilation:
template class ArrayView<int>;
template class ArrayView<float>;
template class ArrayView<String>;
template class ArrayView<String*>;
template class ArrayView<NoncopyableMovable>;

static_assert(Serializable<ArrayView<int>>);
static_assert(Serializable<ArrayView<const int>>);
static_assert(Serializable<ArrayView<String>>);
static_assert(Serializable<ArrayView<const String>>);
static_assert(!Serializable<ArrayView<NoncopyableMovable>>);
static_assert(Serializable<ArrayView<Pixel>>);

static_assert(!Deserializable<ArrayView<int>>);
static_assert(!Deserializable<ArrayView<String>>);

TEST_CASE("ArrayView contains") {
    CHECK(!ArrayView<int>().contains(0));
    CHECK(!ArrayView<const int>({0, 1, 3, 4}).contains(2));
    CHECK(ArrayView<const int>({0, 1, 3, 4}).contains(0));
    CHECK(ArrayView<const int>({0, 1, 3, 4}).contains(1));
    CHECK(ArrayView<const int>({0, 1, 3, 4}).contains(3));
    CHECK(ArrayView<const int>({0, 1, 3, 4}).contains(4));
}

TEST_CASE("ArrayView.asBytes") {
    const Array<int>                 values = {0, 1, 3, 4};
    const ArrayView<const int>       view(values);
    const ArrayView<const std::byte> bytes = view.asBytes();
    CHECK(bytes.size() == 4 * sizeof(int));
    CHECK(bytes[0] == std::byte(0));
    CHECK(bytes[1] == std::byte(0));
    CHECK(bytes[2] == std::byte(0));
    CHECK(bytes[3] == std::byte(0));
    CHECK(bytes[4] == std::byte(1));
    CHECK(bytes[5] == std::byte(0));
    CHECK(bytes[6] == std::byte(0));
    CHECK(bytes[7] == std::byte(0));
    CHECK(bytes[8] == std::byte(3));
    CHECK(bytes[9] == std::byte(0));
    CHECK(bytes[10] == std::byte(0));
    CHECK(bytes[11] == std::byte(0));
    CHECK(bytes[12] == std::byte(4));
    CHECK(bytes[13] == std::byte(0));
    CHECK(bytes[14] == std::byte(0));
    CHECK(bytes[15] == std::byte(0));
}

TEST_CASE("ArrayView.getSub") {
    const Array     values = {1, 2, 3, 4};
    const ArrayView view   = ArrayView<const int>(values);
    CHECK(view.getSub(0) == view);
    CHECK(view.getSub(1) == Array {2, 3, 4});
    CHECK(view.getSub(2) == Array {3, 4});
    CHECK(view.getSub(3) == Array {4});
    CHECK(view.getSub(4) == Array<int> {});
    CHECK_ASSERT(view.getSub(5));
    CHECK_ASSERT(view.getSub(-1));

    CHECK_ASSERT(view.getSub(0, -1));
    CHECK(view.getSub(0, 0) == Array<int> {});
    CHECK(view.getSub(1, 0) == Array<int> {});
    CHECK(view.getSub(2, 0) == Array<int> {});
    CHECK(view.getSub(3, 0) == Array<int> {});

    CHECK(view.getSub(0, 1) == Array<int> {1});
    CHECK(view.getSub(1, 1) == Array<int> {2});
    CHECK(view.getSub(2, 1) == Array<int> {3});
    CHECK(view.getSub(3, 1) == Array<int> {4});

    CHECK(view.getSub(0, 2) == Array<int> {1, 2});
    CHECK(view.getSub(1, 2) == Array<int> {2, 3});
    CHECK(view.getSub(2, 2) == Array<int> {3, 4});
    CHECK_ASSERT(view.getSub(3, 2));

    CHECK(view.getSub(0, 3) == Array<int> {1, 2, 3});
    CHECK(view.getSub(1, 3) == Array<int> {2, 3, 4});
    CHECK_ASSERT(view.getSub(2, 3));
    CHECK_ASSERT(view.getSub(3, 3));

    CHECK(view.getSub(0, 4) == Array<int> {1, 2, 3, 4});
    CHECK_ASSERT(view.getSub(1, 4));
    CHECK_ASSERT(view.getSub(2, 4));
    CHECK_ASSERT(view.getSub(3, 4));

    CHECK_ASSERT(view.getSub(0, 5));
}

BUFF_NAMESPACE_END

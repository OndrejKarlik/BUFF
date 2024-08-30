#include "Lib/TypeTraits.h"
#include "Lib/Pixel.h"
#include "Lib/String.h"

BUFF_NAMESPACE_BEGIN

struct EqualsComparableStruct {
    String x;
    bool   operator==(const EqualsComparableStruct& other) const;
};

struct ThreeWayComparable {
    std::strong_ordering operator<=>(const ThreeWayComparable& other) const;
};

static_assert(IsEqualsComparable<int>);
static_assert(IsEqualsComparable<int, int>);
static_assert(IsEqualsComparable<int, float>);
static_assert(!IsEqualsComparable<int, int*>);

static_assert(IsThreeWayComparable<float>);
static_assert(IsThreeWayComparable<int, float>);
static_assert(IsThreeWayComparable<int, int>);
static_assert(!IsThreeWayComparable<int, int*>);

static_assert(IsEqualsComparable<EqualsComparableStruct>);
static_assert(!IsThreeWayComparable<EqualsComparableStruct>);

static_assert(!IsEqualsComparable<ThreeWayComparable>);
static_assert(IsThreeWayComparable<ThreeWayComparable>);

struct EmptyStruct {};

struct SerializableCustomStruct {
    void serializeCustom(ISerializer& serializer) const;
    void deserializeCustom(IDeserializer& deserializer);
};

struct SerializableSimpleStruct {
    int   x;
    float y;
    void  enumerateStructMembers(auto&& functor) {
        functor(x);
        functor(y);
    }
};

static_assert(Serializable<int>);
static_assert(Serializable<float>);
static_assert(Serializable<bool>);
static_assert(Serializable<String>);
static_assert(Serializable<Pixel>);
static_assert(Serializable<SerializableCustomStruct>);
static_assert(Serializable<SerializableSimpleStruct>);
static_assert(!Serializable<int*>);
static_assert(!Serializable<EqualsComparableStruct>);
static_assert(!Serializable<EmptyStruct>);

static_assert(Deserializable<int>);
static_assert(Deserializable<float>);
static_assert(Deserializable<bool>);
static_assert(Deserializable<String>);
static_assert(Deserializable<Pixel>);
static_assert(Deserializable<SerializableCustomStruct>);
static_assert(Deserializable<SerializableSimpleStruct>);
static_assert(!Deserializable<int*>);
static_assert(!Deserializable<EqualsComparableStruct>);
static_assert(!Deserializable<EmptyStruct>);

enum class WeakEnum { A, B, C };
enum class StrongEnum { A, B, C };
static_assert(Serializable<WeakEnum>);
static_assert(Serializable<StrongEnum>);
static_assert(Deserializable<WeakEnum>);
static_assert(Deserializable<StrongEnum>);

BUFF_NAMESPACE_END

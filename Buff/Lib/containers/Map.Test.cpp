#include "Lib/containers/Map.h"
#include "Lib/Bootstrap.Test.h"
#include "Lib/Json.h"
#include "Lib/Pixel.h"

BUFF_NAMESPACE_BEGIN

struct NoncopyableMovableComparable : NoncopyableMovable {
    int  value;
    bool operator<(const NoncopyableMovableComparable& other) const {
        return value < other.value;
    }
};

// Test compilation:
template class Map<int, int>;
template class Map<float, String>;
template class Map<String, String>;
template class Map<String*, int>;
template class Map<NoncopyableMovableComparable, int>;
template class Map<int, NoncopyableMovable>;

struct CountedKey : NoncopyableMovable {
    inline static int sCount = 0;
    int               value;
    CountedKey(const int value)
        : value(value) {
        ++sCount;
    }
    auto operator<=>(const CountedKey& other) const {
        return value <=> other.value;
    }
    auto operator<=>(const int v) const {
        return value <=> v;
    }
};

TEST_CASE("Map heterogeneous lookup") {
    Map<CountedKey, int> map;
    CHECK(CountedKey::sCount == 0);
    map.insert(CountedKey(10), 100);
    CHECK(CountedKey::sCount == 1);
    map[20] = 200;
    CHECK(CountedKey::sCount == 2);

    CHECK(map.contains(10));
    CHECK(!map.contains(30));
    CHECK(CountedKey::sCount == 2);

    CHECK(*map.find(20) == 200);
    CHECK(CountedKey::sCount == 2);
    CHECK(*map.find(10) == 100);
    CHECK(CountedKey::sCount == 2);

    map.erase(10);
    map.erase(20);
    CHECK(map.isEmpty());
    CHECK(CountedKey::sCount == 2);
}

TEST_CASE("Map serialize") {
    Map<String, Pixel> map; // Try non-trivial type and compound type
    map["abc"] = Pixel(1, 2);
    map["cde"] = Pixel(3, 4);

    JsonSerializer serializer;
    serializer.serialize(map, "map");
    const String state = serializer.getJson(true);
    CHECK(parseJson(state));

    JsonDeserializer   deserializer(state);
    Map<String, Pixel> map2;
    deserializer.deserialize(map2, "map");
    CHECK(map == map2);
}

BUFF_NAMESPACE_END

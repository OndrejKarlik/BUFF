#include "Lib/containers/HashMap.h"
#include "Lib/Bootstrap.Test.h"

BUFF_NAMESPACE_BEGIN

static_assert(Detail::HasGetHashFunction<CachedHash<StringView>>);
// static_assert(Hashable<CachedHash<StringView>>);

struct NoncopyableMovableHashable : NoncopyableMovable {
    int  value;
    bool operator==(const NoncopyableMovableHashable& other) const {
        return value == other.value;
    }
    size_t getHash() const {
        return value;
    }
};

// Test compilation:
template class HashMap<int, int>;
template class HashMap<float, String>;
template class HashMap<String, String>;
template class HashMap<String*, int>;
template class HashMap<NoncopyableMovableHashable, int>;
template class HashMap<int, NoncopyableMovable>;

struct CountedKey : NoncopyableMovable {
    inline static int sCount = 0;
    int               value;
    CountedKey(const int value)
        : value(value) {
        ++sCount;
    }
    bool operator==(const CountedKey& other) const {
        return value == other.value;
    }
    size_t getHash() const {
        return value;
    }
};

struct NoncopyableMovableInt : NoncopyableMovable {
    int value;
    explicit NoncopyableMovableInt(const int x)
        : value(x) {}
    bool operator==(const NoncopyableMovableInt& other) const {
        return value == other.value;
    }
    size_t getHash() const {
        return value;
    }
};

TEST_CASE("HashMap simple") {
    HashMap<int, int> map;
    CHECK(map.isEmpty());
    CHECK(map.size() == 0);
    map[10] = 100;
    CHECK(!map.isEmpty());
    CHECK(map.size() == 1);
    map[20] = 200;
    CHECK(map.size() == 2);
    CHECK(map.contains(10));
    CHECK(map.contains(20));
    CHECK(!map.contains(30));
    map.erase(10);
    CHECK(map.size() == 1);
    CHECK(map.contains(20));
    CHECK(!map.contains(10));
    map.erase(20);
    CHECK(map.isEmpty());
}

TEST_CASE("HashMap iteration") {
    HashMap<int, int> map;
    constexpr int     SIZE = 15;
    for (const int i : range(SIZE)) {
        map[i * 10] = i * 100;
    }
    auto values = map.getAllElementsOrdered();
    CHECK(values.size() == SIZE);
    for (const int i : range(SIZE)) {
        CHECK(*values[i].first == i * 10);
        CHECK(*values[i].second == i * 100);
    }
}

TEST_CASE("HashMap move") {
    HashMap<int, int> map;
    map[10] = 100;
    map[20] = 200;

    auto map2 = std::move(map);
    CHECK(map2[10] == 100);
    CHECK(map2[20] == 200);
    CHECK(map2.size() == 2);
    CHECK(map.size() == 0);
}

TEST_CASE("HashMap Noncopyable") {
    HashMap<NoncopyableMovableInt, NoncopyableMovableInt> map;
    map.insert(NoncopyableMovableInt(10), NoncopyableMovableInt(100));
    map.insert(NoncopyableMovableInt(20), NoncopyableMovableInt(200));
    map.insert(NoncopyableMovableInt(30), NoncopyableMovableInt(300));
    CHECK(map.size() == 3);
    map.erase(NoncopyableMovableInt(20));
    CHECK(map.size() == 2);
    auto map2 = std::move(map);
    CHECK(*map2.find(NoncopyableMovableInt(10)) == NoncopyableMovableInt(100));
    CHECK(*map2.find(NoncopyableMovableInt(30)) == NoncopyableMovableInt(300));
    CHECK(map2.size() == 2);
    CHECK(map.size() == 0);
}

TEST_CASE("HashMap CachedKey") {
    HashMap<StringView, String> map;
    map["a"]                    = "A";
    map["b"]                    = "B";
    constexpr StringView KEY_SV = "a";
    const CachedHash     cached(KEY_SV);
    CHECK(map.findWithCached(cached));
    CHECK(*map.findWithCached(cached) == "A");
}

// TEST_CASE("HashMap heterogeneous lookup") {
//     HashMap<CountedKey, int> map;
//     CHECK(CountedKey::sCount == 0);
//     map.insert(CountedKey(10), 100);
//     CHECK(CountedKey::sCount == 1);
//     map[20] = 200;
//     CHECK(CountedKey::sCount == 2);
//
//     CHECK(map.contains(10));
//     CHECK(!map.contains(30));
//     CHECK(CountedKey::sCount == 2);
//
//     CHECK(*map.find(20) == 200);
//     CHECK(CountedKey::sCount == 2);
//     CHECK(*map.find(10) == 100);
//     CHECK(CountedKey::sCount == 2);
//
//     map.erase(10);
//     map.erase(20);
//     CHECK(map.isEmpty());
//     CHECK(CountedKey::sCount == 2);
// }

BUFF_NAMESPACE_END

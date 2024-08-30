#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/Serialization.h"
#include <map>

BUFF_NAMESPACE_BEGIN

namespace Detail {
template <typename T, typename TMap>
concept MapItemConstructible = ConstructibleFrom<typename TMap::Key, decltype(std::declval<T&>().first)> &&
                               ConstructibleFrom<typename TMap::Value, decltype(std::declval<T&>().second)>;
}

template <LessThanComparable TKey, typename TValue>
class Map {
    std::map<TKey, TValue, std::less<>> mImpl; // std::less<> enables heterogeneous lookup

    struct SerializedItem {
        TKey   key;
        TValue value;
        void   enumerateStructMembers(auto&& functor) {
            functor(key, "key");
            functor(value, "value");
        }
    };

public:
    using Key   = TKey;
    using Value = TValue;

    Map()                                = default;
    Map(const Map& other)                = default;
    Map(Map&& other) noexcept            = default;
    Map& operator=(const Map& other)     = default;
    Map& operator=(Map&& other) noexcept = default;

    template <IterableContainer TContainer>
    explicit Map(TContainer&& container)
        requires Detail::MapItemConstructible<decltype(*std::begin(container)), Map> {
        mImpl.insert_range(std::forward<TContainer>(container));
    }

    void serializeCustom(ISerializer& serializer) const requires Serializable<TKey> && Serializable<TValue> {
        Array<SerializedItem> flat;
        flat.reserve(size());
        for (auto&& [key, value] : mImpl) {
            flat.emplaceBack(key, value);
        }
        serializer.serialize(size(), "size");
        serializer.serializeList(flat, "items");
    }
    void deserializeCustom(IDeserializer& deserializer)
        requires std::copyable<TKey> && std::copyable<TValue> && Deserializable<TKey> &&
                 Deserializable<TValue> {
        int64 newSize;
        deserializer.deserialize(newSize, "size");
        Array<SerializedItem> flat(newSize);
        deserializer.deserializeList(flat, "items");
        mImpl.clear();
        for (auto&& [key, value] : flat) {
            mImpl.emplace(std::move(key), std::move(value));
        }
    }

    bool operator==(const Map& other) const requires EqualsComparable<TKey> && EqualsComparable<TValue>;

    // =======================================================================================================
    // Iterators
    // =======================================================================================================

    auto begin() const {
        return mImpl.cbegin();
    }
    auto end() const {
        return mImpl.cend();
    }
    auto begin() {
        return mImpl.begin();
    }
    auto end() {
        return mImpl.end();
    }

    // =======================================================================================================
    // Queries
    // =======================================================================================================

    bool isEmpty() const {
        return mImpl.empty();
    }
    bool notEmpty() const {
        return !mImpl.empty();
    }

    int64 size() const {
        return mImpl.size();
    }

    template <typename T2>
    bool contains(T2&& key) const requires LessThanComparable<T2, TKey> {
        return mImpl.contains(std::forward<T2>(key));
    }

    template <typename T2>
    const TValue* find(T2&& key) const requires LessThanComparable<T2, TKey> {
        auto it = mImpl.find(std::forward<T2>(key));
        return it != mImpl.end() ? &it->second : nullptr;
    }
    template <typename T2>
    TValue* find(T2&& key) requires LessThanComparable<T2, TKey> {
        auto it = mImpl.find(std::forward<T2>(key));
        return it != mImpl.end() ? &it->second : nullptr;
    }

    /// Creates an element if the key is not already present!
    template <typename T2>
    TValue& operator[](T2&& key)
        requires LessThanComparable<T2, TKey> && std::is_default_constructible_v<TValue> {
        return mImpl[std::forward<T2>(key)];
    }

    // =======================================================================================================
    // Modification
    // =======================================================================================================

    template <IterableContainer T>
    void insertRange(T&& range) requires Detail::MapItemConstructible<decltype(*std::begin(range)), Map> {
        mImpl.insert_range(std::forward<T>(range));
    }

    void insert(TKey key, TValue value) {
        BUFF_ASSERT(!contains(key));
        mImpl.insert(std::pair {std::move(key), std::move(value)});
    }

    void clear() {
        mImpl.clear();
    }

    template <typename T2>
    void erase(T2&& key) requires LessThanComparable<T2, TKey> {
        auto it = mImpl.find(std::forward<T2>(key));
        BUFF_ASSERT(it != mImpl.end());
        mImpl.erase(it);
    }
};

// Needs to be out of class so Map compiles with not yet defined types
template <LessThanComparable TKey, typename TValue>
bool Map<TKey, TValue>::operator==(const Map& other) const
    requires EqualsComparable<TKey> && EqualsComparable<TValue>
    = default;

BUFF_NAMESPACE_END

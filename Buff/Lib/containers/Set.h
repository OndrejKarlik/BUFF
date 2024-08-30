#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include "Lib/containers/Array.h"
#include "Lib/Serialization.h"
#include <set>

BUFF_NAMESPACE_BEGIN

template <LessThanComparable T>
class Set {
    std::set<T, std::less<>> mImpl; // std::less<> enables heterogeneous lookup

public:
    Set()                                = default;
    Set(const Set& other)                = default;
    Set(Set&& other) noexcept            = default;
    Set& operator=(const Set& other)     = default;
    Set& operator=(Set&& other) noexcept = default;

    template <typename T2>
    bool contains(T2&& key) const requires LessThanComparable<T, T2> {
        return mImpl.contains(std::forward<T2>(key));
    }

    bool insert(const T& other) requires std::copyable<T> {
        return mImpl.insert(other).second;
    }

    bool insert(T&& other) {
        return mImpl.insert(std::move(other)).second;
    }

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

    void clear() {
        mImpl.clear();
    }

    int64 size() const {
        return mImpl.size();
    }

    void serializeCustom(ISerializer& serializer) const requires Serializable<T> {
        serializer.serialize(size(), "size");
        serializer.serializeList(mImpl, "items");
    }
    void deserializeCustom(IDeserializer& deserializer) requires Deserializable<T> {
        int64 newSize;
        deserializer.deserialize(newSize, "size");
        Array<T> flat(newSize);
        deserializer.deserializeList(flat, "items");
        mImpl.clear();
        mImpl.insert_range(std::move(flat));
    }
};

BUFF_NAMESPACE_END

#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include <robin_hood.h>

BUFF_NAMESPACE_BEGIN

namespace Detail {
template <typename T>
concept HasGetHashFunction = requires(const T& a) {
    { a.getHash() } -> std::convertible_to<size_t>;
};
template <typename T>
concept HasStdHashOverload = requires(const T& a) {
    { std::hash<T> {}(a) } -> std::convertible_to<size_t>;
};
struct MyHash {
    template <HasGetHashFunction T>
    size_t operator()(const T& x) const {
        return x.getHash();
    }
    template <HasStdHashOverload T>
    size_t operator()(const T& x) const {
        return std::hash<T> {}(x);
    }
};
struct MyEquals {
    template <typename T1, typename T2>
    bool operator()(const T1& x, const T2& y) const {
        return x == y;
    }
};

template <typename T, typename TMap>
concept HashMapItemConstructible =
    ConstructibleFrom<typename TMap::Key, decltype(std::declval<T>().first)> &&
    ConstructibleFrom<typename TMap::Value, decltype(std::declval<T>().second)>;

} // namespace Detail

// Concept for a class with std::hash defined
template <typename T>
concept Hashable = requires(const T& t) {
    { Detail::MyHash {}(t) } -> std::convertible_to<std::size_t>;
    { t == t } -> std::convertible_to<bool>;
};

template <typename TKey>
struct CachedHash {
    const TKey* key  = nullptr;
    size_t      hash = 0;

    CachedHash() = default;

    explicit CachedHash(TKey&& BUFF_UNUSED(key)) = delete;
    explicit CachedHash(const TKey& key)
        : key(&key)
        , hash(Detail::MyHash {}(key)) {}

    bool operator==(const TKey& other) const {
        return *key == other;
    }
    friend bool operator==(const TKey& other, const CachedHash& cached) {
        return *cached.key == other;
    }
    size_t getHash() const {
        BUFF_ASSERT(key != nullptr);
        return hash;
    }
};

template <Hashable TKey, typename TValue>
class HashMap {
    robin_hood::unordered_flat_map<TKey, TValue, Detail::MyHash, Detail::MyEquals> mImpl;

public:
    HashMap()                                    = default;
    HashMap(const HashMap& other)                = default;
    HashMap(HashMap&& other) noexcept            = default;
    HashMap& operator=(const HashMap& other)     = default;
    HashMap& operator=(HashMap&& other) noexcept = default;

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

    Array<std::pair<const TKey*, const TValue*>> getAllElementsOrdered() const requires std::sortable<TKey*> {
        Array<std::pair<const TKey*, const TValue*>> result;
        result.reserve(size());
        for (auto& it : *this) {
            result.emplaceBack(&it.first, &it.second);
        }
        std::ranges::sort(result, [](const auto& a, const auto& b) { return *a.first < *b.first; });
        return result;
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
    bool contains(T2&& key) const {
        return mImpl.contains(std::forward<T2>(key));
    }

    template <typename T2>
    const TValue* find(T2&& key) const {
        auto it = mImpl.find(std::forward<T2>(key));
        return it != mImpl.end() ? &it->second : nullptr;
    }
    template <typename T2>
    TValue* find(T2&& key) {
        auto it = mImpl.find(std::forward<T2>(key));
        return it != mImpl.end() ? &it->second : nullptr;
    }

    TValue* findWithCached(const CachedHash<TKey>& cached) {
        auto it = mImpl.find(cached, robin_hood::is_transparent_tag {});
        return it != mImpl.end() ? &it->second : nullptr;
    }

    /// Creates an element if the key is not already present!
    template <typename T2>
    TValue& operator[](T2&& key) {
        return mImpl[std::forward<T2>(key)];
    }

    // =======================================================================================================
    // Modification
    // =======================================================================================================

    void reserve(const int64 count) {
        BUFF_ASSERT(count >= 0);
        mImpl.reserve(count);
    }

    template <IterableContainer T>
    void insertRange(T&& range)
        requires Detail::HashMapItemConstructible<decltype(*std::begin(range)), HashMap> {
        mImpl.insert(range.begin(), range.end());
    }

    /// Returns reference to the value stored inside the hash map
    TValue& insert(TKey key, TValue value) {
        BUFF_ASSERT(!contains(key));
        return mImpl.emplace(std::move(key), std::move(value)).first->second;
    }

    void clear() {
        mImpl.clear();
    }

    template <typename T2>
    void erase(T2&& key) {
        auto it = mImpl.find(std::forward<T2>(key));
        BUFF_ASSERT(it != mImpl.end());
        mImpl.erase(it);
    }
};

BUFF_NAMESPACE_END

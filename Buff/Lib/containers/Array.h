#pragma once
#include "Lib/Bootstrap.h"
#include "Lib/Concepts.h"
#include "Lib/containers/ArrayView.h"
#include "Lib/containers/Iterator.h"
#include "Lib/Optional.h"
#include "Lib/Serialization.h"
#include "Lib/Utils.h"
#include <vector>

BUFF_NAMESPACE_BEGIN

template <NonConst T>
class Array {
    std::vector<T> mImpl;

public:
    // =======================================================================================================
    // Constructors, basic state
    // =======================================================================================================

    Array()                                                        = default;
    Array(const Array& other) requires std::copyable<T>            = default;
    Array& operator=(const Array& other) requires std::copyable<T> = default;
    Array(Array&& other)                                           = default;
    Array& operator=(Array&& other)                                = default;

    explicit Array(const int64 size)
        : mImpl(size) {}
    Array(const int64 size, const T& initialValue) requires std::copyable<T>
        : mImpl(size, initialValue) {}

    Array(std::initializer_list<T> items) requires std::copyable<T>
        : mImpl(std::move(items)) {}

    template <IterableContainer T2>
    explicit Array(T2&& container) requires std::assignable_from<T&, decltype(*std::declval<T2>().begin())>
        : mImpl(std::begin(container), std::end(container)) {}

    explicit Array(const ArrayView<T> items) requires std::copyable<T>
        : mImpl(items.begin(), items.end()) {}

    explicit Array(const ArrayView<const T> items) requires std::copyable<T>
        : mImpl(items.begin(), items.end()) {}

    int64 size() const {
        return mImpl.size();
    }
    bool isEmpty() const {
        return mImpl.empty();
    }
    bool notEmpty() const {
        return !isEmpty();
    }

    bool operator==(const Array& other) const = default;

    /// Not defined as operator to prevent possible errors (just being able to compare arrays with e.g. < is
    /// wild...
    static auto threeWayCompare(const Array& a, const Array& b) requires std::three_way_comparable<T> {
        return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end());
    }

    // =======================================================================================================
    // Front, back, element access, iterators
    // =======================================================================================================

    typename std::vector<T>::reference front() {
        BUFF_ASSERT(notEmpty());
        return mImpl.front();
    }
    typename std::vector<T>::const_reference front() const {
        BUFF_ASSERT(notEmpty());
        return mImpl.front();
    }
    typename std::vector<T>::reference back() {
        BUFF_ASSERT(notEmpty());
        return mImpl.back();
    }
    typename std::vector<T>::const_reference back() const {
        BUFF_ASSERT(notEmpty());
        return mImpl.back();
    }

    typename std::vector<T>::const_reference operator[](const int64 index) const {
        assertValidIndex(index);
        return mImpl[index];
    }
    typename std::vector<T>::reference operator[](const int64 index) {
        assertValidIndex(index);
        return mImpl[index];
    }

    constexpr Iterator<T> begin() {
        return getIterator<T>(*this, 0);
    }
    constexpr Iterator<T> end() {
        return getIterator<T>(*this, size());
    }
    constexpr Iterator<const T> begin() const {
        return getIterator<const T>(*this, 0);
    }
    constexpr Iterator<const T> end() const {
        return getIterator<const T>(*this, size());
    }

    auto rbegin() {
        return mImpl.rbegin();
    }
    auto rend() {
        return mImpl.rend();
    }
    auto rbegin() const {
        return mImpl.crbegin();
    }
    auto rend() const {
        return mImpl.crend();
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator ArrayView<T>() {
        return ArrayView<T>(mImpl.data(), size());
    }

    // ReSharper disable once CppNonExplicitConversionOperator
    operator ArrayView<const T>() const {
        return ArrayView<const T>(mImpl.data(), size());
    }

    constexpr ArrayView<const T> getSub(const int64           start,
                                        const Optional<int64> count = NULL_OPTIONAL) const {
        return ArrayView<const T>(*this).getSub(start, count);
    }
    constexpr ArrayView<T> getSub(const int64 start, const Optional<int64> count = NULL_OPTIONAL) {
        return ArrayView<T>(*this).getSub(start, count);
    }

    T* data() {
        return mImpl.data();
    }

    const T* data() const {
        return mImpl.data();
    }

    // =======================================================================================================
    // Search
    // =======================================================================================================

    template <typename T2> // heterogeneous lookup
    bool contains(T2&& value) const requires EqualsComparable<T, T2> {
        return find(value).hasValue();
    }

    template <typename T2> // heterogeneous lookup
    Optional<int64> find(T2&& value) const requires EqualsComparable<T, T2> {
        for (const int64 i : range(size())) {
            if (mImpl[i] == value) {
                return i;
            }
        }
        return NULL_OPTIONAL;
    }

    // =======================================================================================================
    // Modifying beginning/end
    // =======================================================================================================

    void pushBack(const T& value) requires std::copyable<T> {
        mImpl.push_back(value);
    }
    void pushBack(T&& value) {
        mImpl.push_back(std::move(value));
    }
    void pushBack() requires std::is_default_constructible_v<T> {
        mImpl.emplace_back();
    }

    void pushFront(const T& value) requires std::copyable<T> {
        mImpl.insert(mImpl.begin(), value);
    }
    void pushFront(T&& value = {}) {
        mImpl.insert(mImpl.begin(), std::move(value));
    }

    void pushBackRange(const ArrayView<const T>& list) requires std::copyable<T> {
        insertRange(size(), list);
    }

    template <typename... TConstructorArgs>
    void emplaceBack(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        mImpl.emplace_back(std::forward<TConstructorArgs>(args)...);
    }

    template <typename... TConstructorArgs>
    void emplaceFront(TConstructorArgs&&... args) requires ConstructibleFrom<T, TConstructorArgs...> {
        mImpl.emplace(mImpl.begin(), std::forward<TConstructorArgs>(args)...);
    }

    T popBack() {
        BUFF_ASSERT(notEmpty());
        T tmp = std::move(mImpl.back());
        mImpl.pop_back();
        return tmp;
    }

    // =======================================================================================================
    // Misc modifications
    // =======================================================================================================

    void resize(const int64 size) {
        mImpl.resize(size);
    }
    void reserve(const int64 size) {
        mImpl.reserve(size);
    }
    void clear() {
        mImpl.clear();
    }
    void fill(const T& value) requires std::copyable<T> {
        std::fill(mImpl.begin(), mImpl.end(), value);
    }

    void insert(const int64 index, const T& value) requires std::copyable<T> {
        BUFF_ASSERT(index >= 0 && index <= size());
        mImpl.insert(mImpl.begin() + index, value);
    }
    void insert(const int64 index, T&& value) {
        BUFF_ASSERT(index >= 0 && index <= size());
        mImpl.insert(mImpl.begin() + index, std::move(value));
    }

    template <typename T2>
    void insertRange(const int64 index, const ArrayView<T2> value) {
        BUFF_ASSERT(index >= 0 && index <= size());
        // emscripten clang does not have std::vector<>::insert_range...
        mImpl.insert(mImpl.begin() + index, value.begin(), value.end());
    }

    void eraseRange(const int64 index, const int64 count) {
        BUFF_ASSERT(index >= 0 && index <= size());
        BUFF_ASSERT(count >= 0);
        BUFF_ASSERT(index + count <= size());
        mImpl.erase(mImpl.begin() + index, mImpl.begin() + index + count);
    }

    void eraseByIndex(const int64 index) {
        assertValidIndex(index);
        mImpl.erase(mImpl.begin() + index);
    }

    template <typename T2> // heterogeneous lookup
    void eraseByValue(const T2& value) requires EqualsComparable<T, T2> {
        const Optional<int64> index = find(value);
        BUFF_ASSERT(index && "eraseByValue did not find requested item!");
        eraseByIndex(*index);
    }

    template <typename TPredicate>
    void eraseIf(TPredicate&& predicate) {
        std::erase_if(mImpl, std::forward<TPredicate>(predicate));
    }

    template <typename T2>
    void replaceRange(const int64 index, const int64 count, const ArrayView<T2> replacement) {
        BUFF_ASSERT(index >= 0 && index <= size());
        BUFF_ASSERT(count >= 0);
        BUFF_ASSERT(index + count <= size());
        const int64 newCount      = replacement.size();
        const int64 directReplace = min(count, newCount);
        for (int i = 0; i < directReplace; ++i) {
            mImpl[index + i] = replacement[i];
        }
        if (newCount >= count) {
            insertRange(index + directReplace, replacement.getSub(directReplace));
        } else {
            eraseRange(index + directReplace, count - directReplace);
        }
    }

    // =======================================================================================================
    // Misc
    // =======================================================================================================

    void serializeCustom(ISerializer& serializer) const requires Serializable<T> {
        serializer.serialize(size(), "size");
        serializer.serializeList(mImpl, "items");
    }
    void deserializeCustom(IDeserializer& deserializer) requires Deserializable<T> {
        int64 newSize;
        deserializer.deserialize(newSize, "size");
        mImpl.resize(newSize);
        deserializer.deserializeList(mImpl, "items");
    }

private:
    void assertValidIndex(const int64 i) const {
        BUFF_ASSERT(i >= 0 && i < size(), i, size());
    }

    template <typename TMaybeConstT, typename TMaybeConstArray>
    static Iterator<TMaybeConstT> getIterator(TMaybeConstArray& array, [[maybe_unused]] const int64 index) {
        TMaybeConstT* base = array.mImpl.data();
#if BUFF_DEBUG
        return Iterator<TMaybeConstT>(base + index, base, base + array.size());
#else
        return Iterator<TMaybeConstT>(base + index);
#endif
    }
};

BUFF_NAMESPACE_END

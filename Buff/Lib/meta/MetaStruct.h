#pragma once
#include "Lib/AutoPtr.h"
#include "Lib/Bootstrap.h"
#include "Lib/containers/Array.h"
#include "Lib/containers/Map.h"
#include "Lib/Exception.h"
#include "Lib/Path.h"
#include "Lib/Pixel.h"
#include <ranges>

BUFF_NAMESPACE_BEGIN

// TODO:
// - what should be inner types, what should be standalone types
// - serialization
// - default values
// - copyability
// - custom types

template <bool TConst>
class GenericPropertyValue;
struct PropertyDefinition;
class JsValue;

using MutableGenericPropertyValue = GenericPropertyValue<false>;
using ConstGenericPropertyValue   = GenericPropertyValue<true>;
using HashedKey                   = uint64;

constexpr HashedKey keyStringHash(const StringView str) {
    // https://stackoverflow.com/questions/1660501/what-is-a-good-64bit-hash-function-in-java-for-textual-strings
    uint64 hash = 1125899906842597L; // prime
    for (auto& ch : str) {
        hash = 31 * hash + static_cast<unsigned char>(ch);
    }
    return hash;
}

enum class PropertyType {
    // When adding new type, update PropertyTypeTraits::sRegistered
    BOOL = 0,
    BYTE,
    INT,
    INT64,
    FLOAT,

    STRING = 1000,
    PIXEL,
    DIRECTORY_PATH,
    FILE_PATH,

    POLYMORPHIC_UNIQUE = 2000,

    // POLYMORPHIC_SHARED, // TODO: Needs serialization with instances tracking

    CUSTOM = 10000,
};

template <typename T>
PropertyType getPropertyType();

template <PropertyType TTypeId>
struct CustomPropertyTypeTraits;

#define BUFF_META_REGISTER_CUSTOM_TYPE(T, id)                                                                \
    template <>                                                                                              \
    struct CustomPropertyTypeTraits<PropertyType(int(PropertyType::CUSTOM) + (id))> {                        \
        using Type = T;                                                                                      \
    };                                                                                                       \
    template <>                                                                                              \
    constexpr PropertyType getPropertyType<T>() {                                                            \
        return PropertyType(int(PropertyType::CUSTOM) + (id));                                               \
    }                                                                                                        \
    inline const TrueType BUFF_CONCATENATE(T, _RegisterHelperDummy_) =                                       \
        PropertyTypeTraits::registerCustomType<T>(getPropertyType<T>())

class PropertyTypeTraits {
    struct Key {
        PropertyType type;
        bool         isList;
        auto         operator<=>(const Key& other) const = default;
    };

public:
    int                   size      = -1;
    int                   alignment = -1;
    const std::type_info* typeInfo  = nullptr;
    // TODO: remove the String parameter?
    std::function<void(const ConstGenericPropertyValue&, ISerializer&, const String&)> serializer;
    std::function<void(MutableGenericPropertyValue&, IDeserializer&, const String&)>   deserializer;

    struct Class {
        std::function<void(void* target)>               defaultConstructor;
        std::function<void(void* target, void* source)> moveConstructor;
        std::function<void(void* target, void* source)> moveAssign;
        std::function<void(void* target)>               destructor;
    };
    Optional<Class> classMethods;

    static const PropertyTypeTraits& get(const PropertyType type, const bool isList) {
        auto* res = getDatabase().find(Key {type, isList});
        BUFF_ASSERT(res, "Trying to get non-registered type.", int(type), isList);
        return *res;
    }

    template <typename T>
    static TrueType registerCustomType(const PropertyType type)
        requires(!std::is_pod_v<T> && Serializable<T> && Deserializable<T>) {
        createVersionsClass<T>(type);
        return {};
    }

    PropertyTypeTraits() = default;

    static bool registerDefaultTypes();

private:
    template <typename T>
    static PropertyTypeTraits createPrimitive() requires std::is_pod_v<T>;
    template <typename T>
    static PropertyTypeTraits createClass() requires(!std::is_pod_v<T>);
    template <typename T>
    static PropertyTypeTraits createList(const PropertyTypeTraits& singleElement);

    template <typename T>
    static void createVersionsClass(const PropertyType type) {
        registerDesc({type, false}, createClass<T>());
        registerDesc({type, true}, createList<T>(createClass<T>()));
    }

    static Map<Key, PropertyTypeTraits>& getDatabase() {
        static Map<Key, PropertyTypeTraits> sValue;
        return sValue;
    }

    static void registerDesc(const Key& key, PropertyTypeTraits&& description);
};

struct PropertyDefinition {
    StringView   name;
    PropertyType type;

    struct Extra {
        bool isList;
    } extra;

    constexpr PropertyDefinition(const StringView   name,
                                 const PropertyType type,
                                 const Extra&       extra = {.isList = false})
        : name(name)
        , type(type)
        , extra(extra) {}
};

struct ListChange {
    enum class Event {
        ADD,
        ERASE,
        MODIFY,
        RESET,
    };
    using enum Event;

    Event           type = Event(-1);
    Optional<int64> index;

    bool operator==(const ListChange& other) const = default;

    JsValue           toJsValue() const;
    static ListChange fromJson(const StringView& json);
};

using MetaStructPropertyObserver     = std::function<bool()>; // return false to unsubscribe
using MetaStructListPropertyObserver = std::function<bool(const ListChange&)>;
using MetaStructObserver             = std::function<bool(StringView)>;

class MetaStructBase : NoncopyableMovable {
protected:
    template <bool TConst>
    friend class ValueWrapperBase;
    template <typename T>
    friend class ListValueWrapper;
    template <bool TConst>
    friend class GenericPropertyValue;
    template <typename T>
    friend class PropertyValueWrapper;
    friend class PropertyTypeTraits;

    class Key {
    protected:
        StringView mName;
        HashedKey  mNameHash;

    public:
        constexpr explicit Key(const StringView name)
            : mName(name)
            , mNameHash(keyStringHash(name)) {}

        StringView getName() const {
            return mName;
        }
        std::strong_ordering operator<=>(const Key& other) const {
            return mNameHash <=> other.mNameHash;
        }
        bool operator==(const Key& other) const {
            return mName == other.mName;
        }
        bool operator==(const StringView other) const {
            return mName == other;
        }
    };

    struct Storage {
        int                                   offset;
        PropertyDefinition                    definition;
        Array<MetaStructPropertyObserver>     observers;
        Array<MetaStructListPropertyObserver> listObservers;

        Storage() = delete;
        Storage(const int offset, const PropertyDefinition& definition)
            : offset(offset)
            , definition(definition) {}
    };
    std::byte*        mAlignedMemory = nullptr;
    Map<Key, Storage> mIdMapping;

    Array<MetaStructObserver> mUniversalObservers;

    ArrayView<const PropertyDefinition> mProperties;

    class ListStorage : public Noncopyable {
        const PropertyTypeTraits& mTraits;
        std::byte*                mMemory   = nullptr;
        int64                     mSize     = 0;
        int64                     mCapacity = 0;

    public:
        explicit ListStorage(const PropertyTypeTraits& traits)
            : mTraits(traits) {}
        ~ListStorage();

        int64 size() const {
            return mSize;
        }
        void resize(const int64 newSize) {
            const int64 oldSize = mSize;
            ensureCapacity(newSize);
            if (mTraits.classMethods) {
                for (int64 i = newSize; i < mSize; ++i) {
                    mTraits.classMethods->destructor(getGeneric(i));
                }
                mSize = newSize;
                for (int64 i = oldSize; i < newSize; ++i) {
                    mTraits.classMethods->defaultConstructor(getGeneric(i));
                }
            } else {
                if (newSize > oldSize) {
                    std::memset(getGenericUnchecked(oldSize), 0, (newSize - oldSize) * mTraits.size);
                }
                mSize = newSize;
            }
        }

        template <typename T>
        void pushBack(T&& value) requires std::is_move_assignable_v<std::decay_t<T>> {
            ensureType<T>();
            resize(mSize + 1);
            get<std::decay_t<T>>(mSize - 1) = std::forward<T>(value);
        }
        void erase(const int64 index) {
            if (mTraits.classMethods) {
                for (int64 i = index; i < mSize - 1; ++i) {
                    mTraits.classMethods->moveAssign(getGeneric(i), getGeneric(i + 1));
                }
            } else {
                std::memmove(getGeneric(index),
                             getGenericUnchecked(index + 1),
                             (mSize - index - 1) * mTraits.size);
            }
            resize(mSize - 1);
        }
        template <typename T>
        ArrayView<const T> getArrayView() const {
            ensureType<T>();
            if (size() == 0) {
                return {};
            } else {
                return ArrayView(&get<T>(0), size());
            }
        }
        template <typename T>
        void assignFromArray(Array<T> array) requires std::is_move_assignable_v<T> {
            ensureType<T>();
            resize(array.size());
            for (int i = 0; i < mSize; ++i) {
                get<T>(i) = std::move(array[i]);
            }
        }

        template <typename T>
        T& get(const int64 index) {
            ensureType<T>();
            BUFF_ASSERT(index >= 0 && index < mSize);
            return reinterpret_cast<T*>(mMemory)[index];
        }
        template <typename T>
        const T& get(const int64 index) const {
            ensureType<T>();
            BUFF_ASSERT(index >= 0 && index < mSize);
            return reinterpret_cast<T*>(mMemory)[index];
        }

    private:
        void ensureCapacity(int64 newCapacity);

        void* getGenericUnchecked(const int64 index) {
            return mMemory + index * mTraits.size;
        }

        void* getGeneric(const int64 index) {
            BUFF_ASSERT(index >= 0 && index < mSize);
            return getGenericUnchecked(index);
        }
        template <typename T>
        void ensureType() const {
            if (*mTraits.typeInfo != typeid(std::decay_t<std::remove_const_t<T>>)) {
                BUFF_STOP;
            }
        }
    };

    explicit MetaStructBase(ArrayView<const PropertyDefinition> properties);

    template <int TSize>
    explicit MetaStructBase(const PropertyDefinition (&properties)[TSize])
        : MetaStructBase(ArrayView(properties, TSize)) {}

    MetaStructBase(MetaStructBase&& other) noexcept {
        std::swap(mAlignedMemory, other.mAlignedMemory);
        std::swap(mIdMapping, other.mIdMapping);
        std::swap(mUniversalObservers, other.mUniversalObservers);
        std::swap(mProperties, other.mProperties);
        BUFF_ASSERT_SIZEOF(sizeof(mAlignedMemory) + sizeof(mIdMapping) + sizeof(mUniversalObservers) +
                           sizeof(mProperties));
    }
    MetaStructBase& operator=(MetaStructBase&& other) noexcept {
        std::swap(mAlignedMemory, other.mAlignedMemory);
        std::swap(mIdMapping, other.mIdMapping);
        std::swap(mUniversalObservers, other.mUniversalObservers);
        std::swap(mProperties, other.mProperties);
        BUFF_ASSERT_SIZEOF(sizeof(mAlignedMemory) + sizeof(mIdMapping) + sizeof(mUniversalObservers) +
                           sizeof(mProperties));
        return *this;
    }

public:
    ~MetaStructBase();

    MutableGenericPropertyValue getUnchecked(StringView key);
    ConstGenericPropertyValue   getUnchecked(StringView key) const;

    void serializeCustom(ISerializer& serializer) const;
    void deserializeCustom(IDeserializer& deserializer);

    // =======================================================================================================
    // Observers
    // =======================================================================================================

    void addObserver(MetaStructObserver observer) {
        mUniversalObservers.pushBack(std::move(observer));
    }

private:
    MutableGenericPropertyValue getUncheckedInternal(const Key& key);
    ConstGenericPropertyValue   getUncheckedInternal(const Key& key) const;

    void checkKey(StringView key) const;

    template <typename T>
    T& checkAndRetrieve(const Key& id) const {
        const Storage& storage = *mIdMapping.find(id);
        const auto&    traits =
            PropertyTypeTraits::get(storage.definition.type, storage.definition.extra.isList);
        BUFF_ASSERT((!storage.definition.extra.isList || std::is_same_v<T, ListStorage>));
        if (*traits.typeInfo == typeid(std::decay_t<T>)) {
            return *reinterpret_cast<T*>(mAlignedMemory + storage.offset);
        } else {
            BUFF_STOP;
        }
    }
    void callObservers(const Key& id, const Optional<ListChange>& listChange = NULL_OPTIONAL) {
        auto callObserversStack = []<typename... TCallArgs>(auto& observersStack, TCallArgs&&... args) {
            for (int i = 0; i < observersStack.size(); ++i) {
                if (!observersStack[i](std::forward<TCallArgs>(args)...)) {
                    observersStack.eraseByIndex(i);
                    --i;
                }
            }
        };
        callObserversStack(mIdMapping.find(id)->observers);
        // If there is no list change specified, we assume that the entire list was reset:
        callObserversStack(mIdMapping.find(id)->listObservers,
                           listChange.valueOr({.type = ListChange::RESET}));
        callObserversStack(mUniversalObservers, id.getName());
    }
};

template <bool TConst>
class ValueWrapperBase {
    friend class MetaStructBase;

protected:
    using Parent = std::conditional_t<TConst, const MetaStructBase, MetaStructBase>;
    Parent&               mParent;
    MetaStructBase::Key   mKey;
    constexpr static bool IS_CONST = TConst;

public:
    ValueWrapperBase(Parent& parent, MetaStructBase::Key key)
        : mParent(parent)
        , mKey(std::move(key)) {}

    // =======================================================================================================
    // Rest
    // =======================================================================================================

    void addObserver(MetaStructPropertyObserver observer) requires(!IS_CONST) {
        mParent.mIdMapping.find(mKey)->observers.pushBack(std::move(observer));
    }

    const PropertyDefinition& getPropertyDefinition() const {
        return mParent.mIdMapping.find(mKey)->definition;
    }

protected:
    template <typename T>
    std::conditional_t<TConst, const T&, T&> get() const {
        BUFF_ASSERT(!getPropertyDefinition().extra.isList);
        return mParent.template checkAndRetrieve<std::decay_t<T>>(mKey);
    }
    std::conditional_t<TConst, const MetaStructBase::ListStorage&, MetaStructBase::ListStorage&> getList()
        const {
        BUFF_ASSERT(this->getPropertyDefinition().extra.isList);
        return this->mParent.template checkAndRetrieve<MetaStructBase::ListStorage>(this->mKey);
    }
};

template <typename T>
class PropertyValueWrapper : public ValueWrapperBase<std::is_const_v<T>> {
    using ValueWrapperBase<std::is_const_v<T>>::IS_CONST;

public:
    PropertyValueWrapper() = delete;
    using ValueWrapperBase<std::is_const_v<T>>::ValueWrapperBase;

    operator const T&() const {
        return this->template get<T>();
    }
    const T& operator*() const {
        return this->template get<T>();
    }
    const T* operator->() const {
        return &this->template get<T>();
    }

    // =======================================================================================================
    // Operations directly on the value
    // =======================================================================================================

    template <typename T2>
    void operator=(T2&& value) requires(!IS_CONST && std::is_assignable_v<T&, T2>) {
        this->template get<T>() = std::forward<T2>(value);
        this->mParent.callObservers(this->mKey);
    }

    template <typename T2>
    bool operator==(const T2& value) const requires IsEqualsComparable<T, T2> {
        return this->template get<T>() == value;
    }

    template <typename T2>
    auto operator<=>(const T2& value) const requires IsThreeWayComparable<T, T2> {
        return this->template get<T>() <=> value;
    }
};

template <typename T>
class ListValueWrapper : public ValueWrapperBase<std::is_const_v<T>> {
    using ValueWrapperBase<std::is_const_v<T>>::IS_CONST;

public:
    ListValueWrapper() = delete;
    using ValueWrapperBase<std::is_const_v<T>>::ValueWrapperBase;

    void pushBack(const T& value) requires(!IS_CONST && std::is_copy_constructible_v<T>) {
        MetaStructBase::ListStorage& list  = this->getList();
        const int64                  index = list.size();
        list.pushBack(value);
        this->mParent.callObservers(this->mKey, ListChange {.type = ListChange::ADD, .index = index});
    }
    void pushBack(T&& value) requires(!IS_CONST && std::is_move_constructible_v<T>) {
        MetaStructBase::ListStorage& list  = this->getList();
        const int64                  index = list.size();
        list.pushBack(std::move(value));
        this->mParent.callObservers(this->mKey, ListChange {.type = ListChange::ADD, .index = index});
    }

    int64 size() const {
        return this->getList().size();
    }

    ArrayView<const T> getArrayView() const {
        return this->getList().template getArrayView<T>();
    }
    void assignFromArray(Array<std::remove_const_t<T>> array) requires(!IS_CONST) {
        this->getList().assignFromArray(std::move(array));
        this->mParent.callObservers(this->mKey, ListChange {.type = ListChange::RESET});
    }

    void erase(const int64 index) requires(!IS_CONST) {
        MetaStructBase::ListStorage& list = this->getList();
        list.erase(index);
        this->mParent.callObservers(this->mKey, ListChange {.type = ListChange::ERASE, .index = index});
    }
    Optional<int64> find(const T& value) const {
        const auto arrayView = getArrayView();
        for (auto&& it : iterate(arrayView)) {
            if (it == value) {
                return it.index();
            }
        }
        return NULL_OPTIONAL;
    }
    template <typename TFunctor>
    Optional<int64> findIf(const TFunctor& fn) const {
        const auto arrayView = getArrayView();
        for (int i = 0; i < arrayView.size(); ++i) {
            if (fn(arrayView[i])) {
                return i;
            }
        }
        return NULL_OPTIONAL;
    }

    const T& operator[](const int64 index) const {
        return this->getList().template get<T>(index);
    }

    void addListObserver(MetaStructListPropertyObserver observer) requires(!IS_CONST) {
        BUFF_ASSERT(this->getPropertyDefinition().extra.isList);
        this->mParent.mIdMapping.find(this->mKey)->listObservers.pushBack(std::move(observer));
    }

    Iterator<const T> begin() const {
        return this->getList().template getArrayView<const T>().begin();
    }
    Iterator<const T> end() const {
        return this->getList().template getArrayView<const T>().end();
    }
    template <IterableContainer TContainer>
    bool operator==(TContainer& value) const requires IsEqualsComparable<T, decltype(*value.begin())> {
        return this->getList().template getArrayView<const T>() == value;
    }
};

/// A temporary object wrapper used for setting/getting a property.
template <bool TConst>
class GenericPropertyValue : public ValueWrapperBase<TConst> {
    friend class MetaStructBase;
    using Parent = std::conditional_t<TConst, const MetaStructBase, MetaStructBase>;

public:
    GenericPropertyValue() = delete;
    using ValueWrapperBase<TConst>::ValueWrapperBase;

    operator ConstGenericPropertyValue() const {
        return ConstGenericPropertyValue {this->mParent, this->mKey};
    }

    void serializeCustom(ISerializer& serializer) const;

    void deserializeCustom(IDeserializer& deserializer) requires(!TConst);

    template <typename T>
    const T& as() const {
        return this->template get<T>();
    }
    template <typename T>
    void assign(T&& value) requires(!TConst) {
        this->template get<std::decay_t<T>>() = std::forward<T>(value);
        this->mParent.callObservers(this->mKey);
    }

    template <typename T>
    ArrayView<const T> asList() const {
        return this->getList().template getArrayView<const T>();
    }
    template <typename T>
    void assignList(Array<T> array) requires(!TConst) {
        this->getList().assignFromArray(std::move(array));
        this->mParent.callObservers(this->mKey, ListChange {.type = ListChange::RESET});
    }
};

extern template class GenericPropertyValue<true>;
extern template class GenericPropertyValue<false>;

template <typename TClass>
class MetaStruct : public MetaStructBase {
    template <typename T>
    struct TypeAsValue {
        using Type = T;
    };
    /// Returns TypeAsValue
    /// Has to be here before usage: clang warns:  warning : use of member 'getType' before its declaration is
    /// a Microsoft extension [-Wmicrosoft-template]
    template <StringLiteral TKey>
    consteval static auto getType() {
        constexpr PropertyType TYPE = getDefinition(TKey.value).type;
        if constexpr (TYPE == PropertyType::BOOL) {
            return TypeAsValue<bool> {};
        } else if constexpr (TYPE == PropertyType::BYTE) {
            return TypeAsValue<std::byte> {};
        } else if constexpr (TYPE == PropertyType::INT) {
            return TypeAsValue<int> {};
        } else if constexpr (TYPE == PropertyType::INT64) {
            return TypeAsValue<int64> {};
        } else if constexpr (TYPE == PropertyType::FLOAT) {
            return TypeAsValue<float> {};
        } else if constexpr (TYPE == PropertyType::STRING) {
            return TypeAsValue<String> {};
        } else if constexpr (TYPE == PropertyType::PIXEL) {
            return TypeAsValue<Pixel> {};
        } else if constexpr (TYPE == PropertyType::FILE_PATH) {
            return TypeAsValue<FilePath> {};
        } else if constexpr (TYPE == PropertyType::DIRECTORY_PATH) {
            return TypeAsValue<DirectoryPath> {};
        } else if constexpr (TYPE == PropertyType::POLYMORPHIC_UNIQUE) {
            return TypeAsValue<AutoPtr<Polymorphic>> {};
        } else {
            return CustomPropertyTypeTraits<TYPE> {};
        }
    }

public:
    MetaStruct()
        : MetaStructBase(TClass::PROPERTIES) {}

    class CheckedKey : public Key {

    public:
        consteval CheckedKey(const StringView name)
            : Key(name) {
            [[maybe_unused]] bool found = false;
            for (auto& i : TClass::PROPERTIES) {
                if (i.name == name) {
                    found = true;
                    break;
                }
            }
            BUFF_ASSERT(found, "Unknown property name");
        }
        consteval CheckedKey(const char* name)
            : CheckedKey(StringView(name)) {}
    };

    template <StringLiteral TKey>
    auto get() {
        using Type = typename decltype(getType<TKey>())::Type;
        if constexpr (!getDefinition(TKey.value).extra.isList) {
            return PropertyValueWrapper<Type>(*this, CheckedKey(TKey.value));
        } else {
            return ListValueWrapper<Type>(*this, CheckedKey(TKey.value));
        }
    }

    template <StringLiteral TKey>
    auto get() const {
        using Type = typename decltype(getType<TKey>())::Type;
        if constexpr (!getDefinition(TKey.value).extra.isList) {
            return PropertyValueWrapper<const Type>(*this, CheckedKey(TKey.value));
        } else {
            return ListValueWrapper<const Type>(*this, CheckedKey(TKey.value));
        }
    }

private:
    consteval static const PropertyDefinition& getDefinition(const char* key) {
        for (auto& i : TClass::PROPERTIES) {
            if (i.name == StringView(key)) {
                return i;
            }
        }
        BUFF_STOP;
    }
};

template <typename T>
PropertyTypeTraits PropertyTypeTraits::createClass() requires(!std::is_pod_v<T>) {
    PropertyTypeTraits result;
    result.size      = sizeof(T);
    result.alignment = alignof(T);
    BUFF_ASSERT(result.alignment <= result.size && result.size % result.alignment == 0);
    result.typeInfo   = &typeid(T);
    result.serializer = [](const ConstGenericPropertyValue& property,
                           ISerializer&                     where,
                           const String& name) { where.serialize(property.as<T>(), name.asCString()); };
    result.deserializer =
        [](MutableGenericPropertyValue& property, IDeserializer& where, const String& name) {
            T value;
            where.deserialize(value, name.asCString());
            property.assign(std::move(value));
        };

    result.classMethods.emplace();
    result.classMethods->defaultConstructor = [](void* target) { new (target) T(); };
    result.classMethods->moveConstructor    = [](void* target, void* from) {
        new (target) T(std::move(*static_cast<T*>(from)));
    };
    result.classMethods->moveAssign = [](void* target, void* from) {
        *static_cast<T*>(target) = std::move(*static_cast<T*>(from));
    };
    result.classMethods->destructor = [](void* target) { static_cast<T*>(target)->~T(); };

    return result;
}
template <typename T>
PropertyTypeTraits PropertyTypeTraits::createList(const PropertyTypeTraits& singleElement) {
    PropertyTypeTraits result;
    result.size      = sizeof(MetaStructBase::ListStorage);
    result.alignment = alignof(MetaStructBase::ListStorage);
    BUFF_ASSERT(result.alignment <= result.size && result.size % result.alignment == 0);
    result.typeInfo = &typeid(MetaStructBase::ListStorage);
    result.serializer =
        [](const ConstGenericPropertyValue& property, ISerializer& where, const String& BUFF_UNUSED(name)) {
            where.serialize(property.asList<T>(), "list");
        };
    result.deserializer =
        [](MutableGenericPropertyValue& property, IDeserializer& where, const String& BUFF_UNUSED(name)) {
            Array<T> tmp;
            where.deserialize(tmp, "list");
            property.assignList(std::move(tmp));
        };

    result.classMethods.emplace();
    result.classMethods->defaultConstructor = [copy = singleElement](void* ptr) {
        new (ptr) MetaStructBase::ListStorage(copy);
    };
    result.classMethods->moveConstructor = [](void* BUFF_UNUSED(target), void* BUFF_UNUSED(from)) {
        // new (target) MmoveetaStructBase::List(*static_cast<const MetaStructBase::List*>(from));
        BUFF_STOP;
    };
    result.classMethods->moveAssign = [](void* BUFF_UNUSED(to), void* BUFF_UNUSED(from)) { BUFF_STOP; };
    result.classMethods->destructor = [](void* ptr) {
        using Tmp = MetaStructBase::ListStorage;
        static_cast<Tmp*>(ptr)->~Tmp();
    };

    return result;
}

BUFF_NAMESPACE_END

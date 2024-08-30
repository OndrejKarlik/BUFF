#include "Lib/meta/MetaStruct.h"
#include "Lib/AutoPtr.h"
#include "Lib/containers/Array.h"
#include "Lib/Json.h"
#include "Lib/Math.h"
#include "Lib/Platform.h"

BUFF_NAMESPACE_BEGIN

// ===========================================================================================================
// MetaStructBase::List
// ===========================================================================================================

static constexpr auto LIST_CHANGE_ENUM_STRINGS =
    std::initializer_list<std::pair<ListChange::Event, const char*>> {
        {ListChange::ADD,    "add"   },
        {ListChange::ERASE,  "erase" },
        {ListChange::MODIFY, "modify"},
        {ListChange::RESET,  "reset" }
};

JsValue ListChange::toJsValue() const {
    const String typeText = [&]() {
        for (auto&& [i, text] : LIST_CHANGE_ENUM_STRINGS) {
            if (this->type == i) {
                return text;
            }
        }
        BUFF_STOP;
    }();
    JsObject result;
    result["type"]  = typeText;
    result["index"] = index.valueOr(-1);
    return result;
}

ListChange ListChange::fromJson(const StringView& json) {
    JsonDeserializer deserializer(json);
    String           typeText;
    deserializer.deserialize(typeText, "type");
    int index;
    deserializer.deserialize(index, "index");
    ListChange result;
    result.type = [&]() {
        for (auto&& [i, text] : LIST_CHANGE_ENUM_STRINGS) {
            if (typeText == text) {
                return i;
            }
        }
        BUFF_STOP;
    }();
    result.index = (index == -1) ? NULL_OPTIONAL : Optional<int64>(index);
    return result;
}

MetaStructBase::ListStorage::~ListStorage() {
    alignedFree(mMemory);
}

void MetaStructBase::ListStorage::ensureCapacity(int64 newCapacity) {
    if (newCapacity <= mCapacity) {
        return;
    }
    newCapacity = max(int64(float(mCapacity) * 1.5f), newCapacity);
    // TODO: implement aligned_realloc
    std::byte* newMemory =
        static_cast<std::byte*>(alignedMalloc(newCapacity * mTraits.size, mTraits.alignment));
    if (mTraits.classMethods) {
        for (int i = 0; i < mSize; ++i) {
            mTraits.classMethods->moveConstructor(newMemory + i * mTraits.size, getGenericUnchecked(i));
        }
    } else {
        std::memcpy(newMemory, mMemory, mSize * mTraits.size);
    }
    mCapacity = newCapacity;
    alignedFree(mMemory);
    mMemory = newMemory;
}

// ===========================================================================================================
// GenericPropertyValue
// ===========================================================================================================

template <bool TConst>
void GenericPropertyValue<TConst>::serializeCustom(ISerializer& serializer) const {
    const PropertyDefinition& info = this->getPropertyDefinition();
    PropertyTypeTraits::get(info.type, info.extra.isList).serializer(*this, serializer, info.name);
}

template <bool TConst>
void GenericPropertyValue<TConst>::deserializeCustom(IDeserializer& deserializer) requires(!TConst) {
    const PropertyDefinition& info = this->getPropertyDefinition();
    PropertyTypeTraits::get(info.type, info.extra.isList).deserializer(*this, deserializer, info.name);
}

template class GenericPropertyValue<true>;
template class GenericPropertyValue<false>;

// ===========================================================================================================
// MetaStructBase
// ===========================================================================================================

MutableGenericPropertyValue MetaStructBase::getUnchecked(const StringView key) {
    checkKey(key);
    return MutableGenericPropertyValue {*this, Key(key)};
}

ConstGenericPropertyValue MetaStructBase::getUnchecked(const StringView key) const {
    checkKey(key);
    return ConstGenericPropertyValue {*this, Key(key)};
}

MutableGenericPropertyValue MetaStructBase::getUncheckedInternal(const Key& key) {
    return MutableGenericPropertyValue {*this, key};
}

ConstGenericPropertyValue MetaStructBase::getUncheckedInternal(const Key& key) const {
    return ConstGenericPropertyValue {*this, key};
}

void MetaStructBase::checkKey(const StringView key) const {
    BUFF_ASSERT(
        std::ranges::any_of(mProperties, [&](const PropertyDefinition& def) { return key == def.name; }));
}

void MetaStructBase::serializeCustom(ISerializer& serializer) const {
    for (const auto& key : mIdMapping | std::views::keys) {
        serializer.serialize(getUncheckedInternal(key), String(key.getName()).asCString());
    }
}
void MetaStructBase::deserializeCustom(IDeserializer& deserializer) {
    for (const auto& key : mIdMapping | std::views::keys) {
        auto tmp = getUncheckedInternal(key);
        deserializer.deserialize(tmp, String(key.getName()).asCString());
    }
}

// TODO: do calculations, only once, share Mapping across instances
MetaStructBase::MetaStructBase(ArrayView<const PropertyDefinition> properties)
    : mProperties(properties) {
    int totalSize    = 0;
    int maxAlignment = 0;
    for (auto& prop : properties) {
        const PropertyTypeTraits& traits = PropertyTypeTraits::get(prop.type, prop.extra.isList);
        maxAlignment                     = max(maxAlignment, traits.alignment);
        totalSize                        = alignUp(totalSize, traits.alignment);
        mIdMapping.insert(Key(prop.name), {totalSize, prop});
        totalSize += traits.size;
    }
    BUFF_ASSERT(mIdMapping.size() == properties.size(), "Duplicate property names or hashing collisions.");
    mAlignedMemory = static_cast<std::byte*>(alignedMalloc(totalSize, maxAlignment));
    std::memset(mAlignedMemory, 0, size_t(totalSize));
    for (auto& i : mIdMapping | std::views::values) {
        const auto& traits = PropertyTypeTraits::get(i.definition.type, i.definition.extra.isList);
        if (traits.classMethods) {
            traits.classMethods->defaultConstructor(mAlignedMemory + i.offset);
        }
    }
}

MetaStructBase::~MetaStructBase() {
    for (auto& i : mIdMapping | std::views::values) {
        const auto& traits = PropertyTypeTraits::get(i.definition.type, i.definition.extra.isList);
        if (traits.classMethods) {
            traits.classMethods->destructor(mAlignedMemory + i.offset);
        }
    }
    alignedFree(mAlignedMemory);
}

// ===========================================================================================================
// PropertyTypeTraits
// ===========================================================================================================

[[maybe_unused]] static const bool REGISTER_DEFAULT_TYPES = PropertyTypeTraits::registerDefaultTypes();

void PropertyTypeTraits::registerDesc(const Key& key, PropertyTypeTraits&& description) {
    BUFF_ASSERT(!getDatabase().contains(key), "Duplicate key");
    getDatabase()[key] = std::move(description);
}

bool PropertyTypeTraits::registerDefaultTypes() {
    static int numCalled = 0;
    BUFF_ASSERT(numCalled++ == 0, "registerDefaultTypes called more than once");
    auto createVersionsPrimitive = [&]<typename T>(const PropertyType type) {
        registerDesc({type, false}, createPrimitive<T>());
        registerDesc({type, true}, createList<T>(createPrimitive<T>()));
    };

    // std::vector<bool> fails here, lol
    registerDesc({PropertyType::BOOL, false}, createPrimitive<bool>());
    // createVersionsPrimitive.operator()<bool>(PropertyType::BOOL);

    createVersionsPrimitive.operator()<std::byte>(PropertyType::BYTE);
    createVersionsPrimitive.operator()<int>(PropertyType::INT);
    createVersionsPrimitive.operator()<int64>(PropertyType::INT64);
    createVersionsPrimitive.operator()<float>(PropertyType::FLOAT);

    createVersionsClass<String>(PropertyType::STRING);
    createVersionsClass<Pixel>(PropertyType::PIXEL);
    createVersionsClass<FilePath>(PropertyType::FILE_PATH);
    createVersionsClass<DirectoryPath>(PropertyType::DIRECTORY_PATH);

    createVersionsClass<AutoPtr<Polymorphic>>(PropertyType::POLYMORPHIC_UNIQUE);
    // createVersionsClass.operator()<SharedPtr<Polymorphic>>(PropertyType::POLYMORPHIC_SHARED);
    return true;
}

template <typename T>
PropertyTypeTraits PropertyTypeTraits::createPrimitive() requires std::is_pod_v<T> {
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
    return result;
}

BUFF_NAMESPACE_END

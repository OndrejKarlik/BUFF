#pragma once
#include "Lib/Bootstrap.h"
#include <functional>
#include <vector>
#ifdef __EMSCRIPTEN__
#    include <type_traits>
#endif

BUFF_NAMESPACE_BEGIN

class StringView;
class ISerializer;
class IDeserializer;
class Polymorphic;

namespace Detail {

enum class PrimitiveSerializationCategory {
    BOOL,
    INT,
    UNSIGNED_INT,
    FLOAT,
    STRING,
};

template <typename T>
constexpr PrimitiveSerializationCategory classify() {
    using DecayedT = std::decay_t<T>;
    if constexpr (std::is_same_v<bool, DecayedT>) {
        return PrimitiveSerializationCategory::BOOL;
    } else if constexpr (std::is_integral_v<DecayedT>) {
        return std::is_signed_v<DecayedT> ? PrimitiveSerializationCategory::INT
                                          : PrimitiveSerializationCategory::UNSIGNED_INT;
    } else if constexpr (std::is_floating_point_v<DecayedT>) {
        return PrimitiveSerializationCategory::FLOAT;
    } else if constexpr (std::is_same_v<DecayedT, String>) {
        return PrimitiveSerializationCategory::STRING;
    } else if constexpr (std::is_enum_v<DecayedT>) {
        return classify<std::underlying_type_t<DecayedT>>();
    } else {
        static_assert(TEMPLATED_FALSE<T>, "Unsupported type for serialization!");
    }
}

} // namespace Detail

// ===========================================================================================================
// Traits, macros
// ===========================================================================================================

template <typename T>
concept HasCustomSerialization = requires(const T& t, ISerializer& serializer, IDeserializer& deserializer) {
    t.serializeCustom(serializer);
};
template <typename T>
concept HasCustomDeserialization = requires(T& t, ISerializer& serializer, IDeserializer& deserializer) {
    t.deserializeCustom(deserializer);
};
template <typename T>
concept HasSimpleStructSerializationDeserialization =
    requires(T t) { t.enumerateStructMembers([](auto&, const char*) {}); };

template <typename T>
concept IsSerializableDeserializablePrimitive =
    std::is_fundamental_v<T> || std::is_enum_v<T> || std::is_same_v<T, String>;

template <typename T>
concept Serializable = HasCustomSerialization<T> || HasSimpleStructSerializationDeserialization<T> ||
                       IsSerializableDeserializablePrimitive<std::remove_const_t<T>>;
template <typename T>
concept Deserializable = HasCustomDeserialization<T> || HasSimpleStructSerializationDeserialization<T> ||
                         IsSerializableDeserializablePrimitive<T>;

// TODO: cancel?
struct TransparentObjectSerialization {};

// ===========================================================================================================
// ISerializer/IDeserializer
// ===========================================================================================================

class ISerializer
    : public Polymorphic
    , public Noncopyable {
public:
    template <Serializable T>
    void serialize(const T& value, const char* name) {
        constexpr bool TRANSPARENT_OBJECT =
            std::is_base_of_v<TransparentObjectSerialization, std::decay_t<T>>;
        setPropertyName(name);
        if constexpr (HasCustomSerialization<T>) {
            if constexpr (!TRANSPARENT_OBJECT) {
                pushObject();
            }
            value.serializeCustom(*this);
            if constexpr (!TRANSPARENT_OBJECT) {
                popObject();
            }
        } else if constexpr (HasSimpleStructSerializationDeserialization<T>) {
            if constexpr (!TRANSPARENT_OBJECT) {
                pushObject();
            }
            const_cast<T&>(value).enumerateStructMembers(
                [this](auto& field, const char* fieldName) { this->serialize(field, fieldName); });
            if constexpr (!TRANSPARENT_OBJECT) {
                popObject();
            }
        } else {
            serializePrimitive(&value, sizeof(value), Detail::classify<T>());
        }
    }

    template <typename T>
    void serializeList(const T& list, const char* name)
        requires Serializable<std::decay_t<decltype(*list.begin())>> {
        setPropertyName(name);
        auto firstIt = list.begin();
        serializeListImpl([&]() {
            if (firstIt == list.end()) {
                return false;
            }
            serialize(*firstIt, nullptr);
            ++firstIt;
            return true;
        });
    }

private:
    virtual void pushObject() {}
    virtual void popObject() {}
    virtual void setPropertyName(const char* BUFF_UNUSED(name)) {}
    virtual void serializePrimitive(const void*                            data,
                                    int                                    size,
                                    Detail::PrimitiveSerializationCategory type) = 0;
    virtual void serializeListImpl(const std::function<bool()>& itemFunctor)     = 0;
};

class IDeserializer
    : public Polymorphic
    , public Noncopyable {
public:
    template <Deserializable T>
    void deserialize(T& value, const char* name) {
        constexpr bool TRANSPARENT_OBJECT = std::is_base_of_v<TransparentObjectSerialization, T>;
        setPropertyName(name);
        if constexpr (HasCustomSerialization<T>) {
            if constexpr (!TRANSPARENT_OBJECT) {
                pushObject();
            }
            value.deserializeCustom(*this);
            if constexpr (!TRANSPARENT_OBJECT) {
                popObject();
            }
        } else if constexpr (HasSimpleStructSerializationDeserialization<T>) {
            if constexpr (!TRANSPARENT_OBJECT) {
                pushObject();
            }
            value.enumerateStructMembers(
                [this](auto& field, const char* fieldName) { this->deserialize(field, fieldName); });
            if constexpr (!TRANSPARENT_OBJECT) {
                popObject();
            }
        } else {
            deserializePrimitive(&value, sizeof(value), Detail::classify<T>());
        }
    }
    template <typename T>
    void deserializeList(T& list, const char* name)
        requires Deserializable<std::decay_t<decltype(*list.begin())>> {
        setPropertyName(name);
        auto firstIt = list.begin();
        deserializeListImpl([&]() {
            if (firstIt == list.end()) {
                return false;
            }
            deserialize(*firstIt, nullptr);
            ++firstIt;
            return true;
        });
    }

private:
    virtual void pushObject() {}
    virtual void popObject() {}
    virtual void setPropertyName(const char* BUFF_UNUSED(name)) {}
    virtual void deserializePrimitive(void* data, int size, Detail::PrimitiveSerializationCategory type) = 0;
    virtual void deserializeListImpl(const std::function<bool()>& itemFunctor)                           = 0;
};

// ===========================================================================================================
// BinarySerializer/BinaryDeserializer
// ===========================================================================================================

using BinarySerializationState = std::vector<std::byte>;

class BinarySerializer final : public ISerializer {
    BinarySerializationState mBytes;

public:
    const BinarySerializationState& getState() const {
        return mBytes;
    }

private:
    virtual void serializePrimitive(const void*                            data,
                                    int                                    size,
                                    Detail::PrimitiveSerializationCategory type) override;
    virtual void serializeListImpl(const std::function<bool()>& itemFunctor) override;
};

class BinaryDeserializer final : public IDeserializer {
    BinarySerializationState mBytes;
    int64                    mPtr = 0;

public:
    explicit BinaryDeserializer(BinarySerializationState serialized)
        : mBytes(std::move(serialized)) {}

private:
    virtual void deserializePrimitive(void*                                  data,
                                      int                                    size,
                                      Detail::PrimitiveSerializationCategory type) override;
    virtual void deserializeListImpl(const std::function<bool()>& itemFunctor) override;
};

// ===========================================================================================================
// JsonSerializer/JsonDeserializer
// ===========================================================================================================

class JsonSerializer final : public ISerializer {
    struct Impl;
    std::unique_ptr<Impl> mImpl;

public:
    JsonSerializer();
    virtual ~JsonSerializer() override;
    String getJson(bool addNewlines) const;

private:
    virtual void pushObject() override;
    virtual void popObject() override;
    virtual void setPropertyName(const char* name) override;
    virtual void serializePrimitive(const void*                            data,
                                    int                                    size,
                                    Detail::PrimitiveSerializationCategory type) override;
    virtual void serializeListImpl(const std::function<bool()>& itemFunctor) override;
};

class JsonDeserializer final : public IDeserializer {
    struct Impl;
    std::unique_ptr<Impl> mImpl;

public:
    explicit JsonDeserializer(const String& serialized);
    virtual ~JsonDeserializer() override;

private:
    virtual void pushObject() override;
    virtual void popObject() override;
    virtual void setPropertyName(const char* name) override;
    virtual void deserializePrimitive(void*                                  data,
                                      int                                    size,
                                      Detail::PrimitiveSerializationCategory type) override;
    virtual void deserializeListImpl(const std::function<bool()>& itemFunctor) override;
};

// ===========================================================================================================
// Polymorphic serialization
// ===========================================================================================================

namespace Detail {

void registerForPolymorphicSerializationImpl(const char* name,
                                             Polymorphic* (*constructor)(),
                                             void (*serializeT)(const Polymorphic*,
                                                                ISerializer&,
                                                                const char*),
                                             void (*deserializeT)(Polymorphic*, IDeserializer&, const char*));

Polymorphic* instantiatePolymorphicClass(const char* name, IDeserializer& deserializer);

void serializePolymorphicClass(const char* name, const Polymorphic& value, ISerializer& serializer);

template <typename T>
bool registerForPolymorphicSerialization() {
    static_assert(std::is_base_of_v<Polymorphic, T>);
    static_assert(Serializable<T>);
    static_assert(Deserializable<T>);
    registerForPolymorphicSerializationImpl(
        typeid(T).name(),
        []() -> Polymorphic* { return new T; },
        [](const Polymorphic* toSerialize, ISerializer& serializer, const char* name) {
            serializer.serialize(*dynamic_cast<const T*>(toSerialize), name);
        },
        [](Polymorphic* toDeserialize, IDeserializer& deserializer, const char* name) {
            deserializer.deserialize(*dynamic_cast<T*>(toDeserialize), name);
        });
    return true;
}
}

// Use inside polymorphic class
#define BUFF_REGISTER_FOR_POLYMORPHIC_SERIALIZATION(T)                                                       \
    static inline const auto dummy_ = Detail::registerForPolymorphicSerialization<T>()

BUFF_NAMESPACE_END

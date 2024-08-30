#include "Lib/Serialization.h"
#include "Lib/containers/Set.h"
#include "Lib/Exception.h"
#include "Lib/Function.h"
#include "Lib/Json.h"
#include "Lib/String.h"
#include <functional>
#include <map>

BUFF_NAMESPACE_BEGIN

struct RegisteredClass {
    Function<Polymorphic*()>                                      constructor;
    Function<void(const Polymorphic*, ISerializer&, const char*)> serialize;
    Function<void(Polymorphic*, IDeserializer&, const char*)>     deserialize;
};

struct RegisteredClasses {
    std::map<String, RegisteredClass> classes;
};

static auto& registeredClasses() {
    static RegisteredClasses sClasses;
    return sClasses.classes;
}

void Detail::registerForPolymorphicSerializationImpl(
    const char* name,
    Polymorphic* (*constructor)(),
    void (*serializeT)(const Polymorphic*, ISerializer&, const char*),
    void (*deserializeT)(Polymorphic*, IDeserializer&, const char*)) {
    BUFF_ASSERT(!registeredClasses().contains(String(name)));
    registeredClasses()[name] = RegisteredClass {constructor, serializeT, deserializeT};
}

void Detail::serializePolymorphicClass(const char* name, const Polymorphic& value, ISerializer& serializer) {
    BUFF_ASSERT(registeredClasses().contains(name));
    const auto& entry = registeredClasses().find(name)->second;
    entry.serialize(&value, serializer, ("Polymorphic_"_S + name).asCString());
}

Polymorphic* Detail::instantiatePolymorphicClass(const char* name, IDeserializer& deserializer) {
    BUFF_ASSERT(registeredClasses().contains(name));
    const auto&  entry  = registeredClasses().find(name)->second;
    Polymorphic* result = entry.constructor();
    entry.deserialize(result, deserializer, ("Polymorphic_"_S + name).asCString());
    return result;
}

// ===========================================================================================================
// BinarySerializer/Deserializer
// ===========================================================================================================

void BinarySerializer::serializePrimitive(const void*                                  data,
                                          const int                                    size,
                                          const Detail::PrimitiveSerializationCategory type) {
    const int64 oldVectorSize = int64(mBytes.size());
    if (type == Detail::PrimitiveSerializationCategory::STRING) {
        const String& string = *static_cast<const String*>(data);
        serialize(string.size(), "size");
        // Serialization category does not matter here:
        serializePrimitive(string.asCString(), string.size(), Detail::PrimitiveSerializationCategory(-1));
    } else {
        mBytes.resize(mBytes.size() + size);
        std::memcpy(mBytes.data() + oldVectorSize, data, size);
    }
}
void BinarySerializer::serializeListImpl(const std::function<bool()>& itemFunctor) {
    while (itemFunctor()) {
    }
}

void BinaryDeserializer::deserializePrimitive(void*                                        data,
                                              int                                          size,
                                              const Detail::PrimitiveSerializationCategory type) {
    if (size < 0) {
        throw Exception("Negative size deserialization");
    }
    if (type == Detail::PrimitiveSerializationCategory::STRING) {
        String& string = *static_cast<String*>(data);
        deserialize(size, "size"); // Get actual size
        if (size < 0) {
            throw Exception("Negative size deserialization");
        }
        if (int64(mBytes.size()) < mPtr + size) {
            throw Exception("Not enough data to deserialize");
        }
        const StringView view(reinterpret_cast<const char*>(mBytes.data()) + mPtr, size);
        mPtr += size;
        string = view;
    } else {
        if (int64(mBytes.size()) < mPtr + size) {
            throw Exception("Not enough data to deserialize");
        }
        std::memcpy(data, mBytes.data() + mPtr, size);
        mPtr += size;
    }
}
void BinaryDeserializer::deserializeListImpl(const std::function<bool()>& itemFunctor) {
    while (itemFunctor()) {
    }
}

// ===========================================================================================================
// JsonSerializer
// ===========================================================================================================

struct JsonSerializer::Impl {
    JsValue         root;
    Array<JsValue*> stack;
    String          currentName;

    Impl() {
        root = JsObject();
        stack.pushBack(&root);
    }
    const String& getUniqueCurrentName() {
        BUFF_ASSERT(!currentName.isEmpty());
        BUFF_ASSERT(!stack.back()->contains(currentName));
        // if (currentName.isEmpty()) {
        //      currentName = "_" + toStr(stack.back()->size());
        //  }
        return currentName;
    }
    bool isSerializingList() const {
        return stack.back()->isArray();
    }
};

JsonSerializer::JsonSerializer()
    : mImpl(std::make_unique<Impl>()) {}

JsonSerializer::~JsonSerializer() = default;

void JsonSerializer::pushObject() {
    auto& back = *mImpl->stack.back();
    if (mImpl->isSerializingList()) {
        back.push(JsObject());
        mImpl->stack.pushBack(&back.peek());
    } else {
        auto& obj = back[mImpl->getUniqueCurrentName()] = JsObject();
        mImpl->stack.pushBack(&obj);
    }
}

void JsonSerializer::popObject() {
    mImpl->stack.popBack();
    BUFF_ASSERT(mImpl->stack.notEmpty());
    setPropertyName(nullptr);
}

String JsonSerializer::getJson(const bool addNewlines) const {
    BUFF_ASSERT(mImpl->stack.size() == 1);
    return mImpl->root.toJson(addNewlines ? Optional(0) : NULL_OPTIONAL);
}

void JsonSerializer::setPropertyName(const char* name) {
    mImpl->currentName = name ? name : "";
}

void JsonSerializer::serializePrimitive(const void*                                  data,
                                        const int                                    size,
                                        const Detail::PrimitiveSerializationCategory type) {
    BUFF_ASSERT(data);
    auto getValue = [&]() -> JsValue {
        switch (type) {
        case Detail::PrimitiveSerializationCategory::BOOL:
            return *static_cast<const bool*>(data);
        case Detail::PrimitiveSerializationCategory::INT:
            switch (size) {
            case 1:
                return int64(*static_cast<const int8*>(data));
            case 2:
                return int64(*static_cast<const int16*>(data));
            case 4:
                return int64(*static_cast<const int*>(data));
            case 8:
                return *static_cast<const int64*>(data);
            default:
                BUFF_STOP;
            }
        case Detail::PrimitiveSerializationCategory::UNSIGNED_INT:
            switch (size) {
            case 1:
                return uint64(*static_cast<const uint8*>(data));
            case 2:
                return uint64(*static_cast<const uint16*>(data));
            case 4:
                return uint64(*static_cast<const uint*>(data));
            case 8:
                return *static_cast<const uint64*>(data);
            default:
                BUFF_STOP;
            }
        case Detail::PrimitiveSerializationCategory::FLOAT:
            switch (size) {
            case 4:
                return double(*static_cast<const float*>(data));
            case 8:
                return *static_cast<const double*>(data);
            default:
                BUFF_STOP;
            }
        case Detail::PrimitiveSerializationCategory::STRING:
            return *static_cast<const String*>(data);
        default:
            BUFF_STOP;
        }
    };

    auto& back = *mImpl->stack.back();
    if (mImpl->isSerializingList()) {
        back.push(getValue());
    } else {
        back[mImpl->getUniqueCurrentName()] = getValue();
    }
}

void JsonSerializer::serializeListImpl(const std::function<bool()>& itemFunctor) {
    auto& array = (*mImpl->stack.back())[mImpl->getUniqueCurrentName()] = JsArray();
    mImpl->stack.pushBack(&array);
    while (itemFunctor()) {
    }
    mImpl->stack.popBack();
}

// ===========================================================================================================
// JsonDeserializer
// ===========================================================================================================

struct JsonDeserializer::Impl {
    const JsValue root;
    struct StackItem {
        const JsValue* value;
        int            arrayIndex = 0;
    };
    Array<StackItem> stack;
    String           currentName;

    bool isReadingList() const {
        BUFF_ASSERT(stack.back().value);
        return stack.back().value->isArray();
    }
};

JsonDeserializer::JsonDeserializer(const String& serialized)
    : mImpl(std::make_unique<Impl>(*parseJson(serialized))) {
    mImpl->stack.pushBack({&mImpl->root});
}

JsonDeserializer::~JsonDeserializer() = default;

void JsonDeserializer::pushObject() {
    auto& back = mImpl->stack.back();
    if (mImpl->isReadingList()) {
        mImpl->stack.pushBack({&(*back.value)[back.arrayIndex++]});
    } else {
        const JsValue* found = back.value->find(mImpl->currentName);
        BUFF_ASSERT(found);
        mImpl->stack.pushBack({found});
    }
}

void JsonDeserializer::popObject() {
    mImpl->stack.popBack();
}

void JsonDeserializer::setPropertyName(const char* name) {
    mImpl->currentName = name ? name : "";
}

void JsonDeserializer::deserializePrimitive(void*                                        data,
                                            const int                                    size,
                                            const Detail::PrimitiveSerializationCategory type) {

    const JsValue& value = [&]() -> const JsValue& {
        auto& back = mImpl->stack.back();
        if (mImpl->isReadingList()) {
            return (*back.value)[back.arrayIndex++];
        } else {
            return *back.value->find(mImpl->currentName);
        }
    }();
    switch (type) {
    case Detail::PrimitiveSerializationCategory::BOOL:
        *static_cast<bool*>(data) = value.get<bool>();
        break;
    case Detail::PrimitiveSerializationCategory::INT:
        switch (size) {
        case 1:
            *static_cast<int8*>(data) = int8(double(value));
            break;
        case 2:
            *static_cast<int16*>(data) = int16(double(value));
            break;
        case 4:
            *static_cast<int*>(data) = int(double(value));
            break;
        case 8:
            *static_cast<int64*>(data) = int64(double(value));
            break;
        default:
            BUFF_STOP;
        }
        break;
    case Detail::PrimitiveSerializationCategory::UNSIGNED_INT:
        switch (size) {
        case 1:
            *static_cast<uint8*>(data) = uint8(double(value));
            break;
        case 2:
            *static_cast<uint16*>(data) = uint16(double(value));
            break;
        case 4:
            *static_cast<uint*>(data) = uint(double(value));
            break;
        case 8:
            *static_cast<uint64*>(data) = uint64(double(value));
            break;
        default:
            BUFF_STOP;
        }
        break;
    case Detail::PrimitiveSerializationCategory::FLOAT:
        switch (size) {
        case 4:
            *static_cast<float*>(data) = float(double(value));
            break;
        case 8:
            *static_cast<double*>(data) = value;
            break;
        default:
            BUFF_STOP;
        }
        break;
    case Detail::PrimitiveSerializationCategory::STRING:
        *static_cast<String*>(data) = StringView(value);
        break;
    default:
        BUFF_STOP;
    }
}

void JsonDeserializer::deserializeListImpl(const std::function<bool()>& itemFunctor) {
    const int64 stackSize = mImpl->stack.size();
    mImpl->stack.pushBack({mImpl->stack.back().value->find(mImpl->currentName)});
    while (itemFunctor()) {
    }
    mImpl->stack.popBack();
    BUFF_ASSERT(stackSize == mImpl->stack.size());
}

BUFF_NAMESPACE_END
